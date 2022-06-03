/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_pepper
 * @{
 *
 * @file
 * @brief       PrEcise Privacy-PresERving Proximity Tracing (PEPPER) implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "pepper.h"

#include "event.h"
#include "event/thread.h"
#include "event/periodic.h"
#include "event/callback.h"

#include "ztimer.h"
#include "timex.h"
#include "fmt.h"

#include "epoch.h"
#include "ed.h"
#if IS_USED(MODULE_TWR)
#include "twr.h"
#endif
#include "ebid.h"
#include "desire_ble_adv.h"
#include "desire_ble_scan.h"
#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

#if IS_USED(MODULE_PEPPER_SRV)
#include "pepper_srv.h"
#endif

static controller_t _controller = {
    .lock = MUTEX_INIT,
#if IS_USED(MODULE_TWR)
    .twr_params = {
        .rx_offset_ticks = CONFIG_TWR_RX_OFFSET_TICKS,
        .tx_offset_ticks = CONFIG_TWR_TX_OFFSET_TICKS
    }
#endif
};

static uint32_t pepper_sec_since_start(void)
{

    return ztimer_now(ZTIMER_SEC) - _controller.start_time;
}
#if IS_USED(MODULE_TWR)
static bool _twr_should_listen(uint32_t timestamp, ed_t *ed)
{
    /* check if no successfull TWR exchange in last CONFIG_PEPPER_TWR_BACK_OFF_S */
    if ((ed->uwb.seen_last_rx_s + _controller.twr_params.backoff <= timestamp) ||
        ed->uwb.seen_last_rx_s == 0) {
        return true;
    }
    LOG_INFO("[pepper]: lst skip 0x%04" PRIx16 ", %" PRIu32 "s < %" PRIu16 "s\n",
             ed_get_short_addr(
                 ed), timestamp - ed->uwb.seen_last_rx_s, _controller.twr_params.backoff);
    return false;
}

static bool _twr_should_request(uint32_t timestamp, ed_t *ed)
{
    if (ed->ebid.status.status == EBID_HAS_ALL) {
        /* 1. check if advertisement where received from neighbor in last CONFIG_MIA_TIME_S */
        if (ed->seen_last_s + CONFIG_MIA_TIME_S > timestamp) {
            /* 1.1 check if no successfull TWR exchange in last CONFIG_PEPPER_TWR_BACK_OFF_S */
            if ((ed->uwb.seen_last_s + _controller.twr_params.backoff <= timestamp + 1) ||
                ed->uwb.seen_last_s == 0) {
                return true;
            }
            else {
                LOG_INFO(
                    "[pepper]: req skip 0x%04" PRIx16 ": %" PRIu32 "s < %" PRIu16 "s\n",
                    ed_get_short_addr(
                        ed), timestamp + 1 - ed->uwb.seen_last_s, _controller.twr_params.backoff);
            }
        }
        else {
            LOG_WARNING("[pepper]: req skip encounter, missing over BLE\n");
        }
    }
    return false;
}

static uint16_t _get_twr_offset(ebid_t *ebid, uint16_t seed)
{
    /* last two bytes of the EBID */
    uint32_t offset_ms = (ebid->parts.ebid.u8[0] + (ebid->parts.ebid.u8[1] << 8)) ^ seed;

    /* add a minimum offset and a random EBID based one */
    offset_ms = (offset_ms % CONFIG_BLE_ADV_ITVL_MS) + CONFIG_TWR_MIN_OFFSET_MS;
    /* TODO: add some RIOT functions for this */
    return os_cputime_usecs_to_ticks(offset_ms * US_PER_MS);
}

static uint16_t _get_twr_rx_offset(ebid_t *ebid, uint16_t seed)
{
    return _get_twr_offset(ebid, seed) + _controller.twr_params.rx_offset_ticks;
}

static uint16_t _get_twr_tx_offset(ebid_t *ebid, uint16_t seed)
{
    return _get_twr_offset(ebid, seed) + _controller.twr_params.tx_offset_ticks;
}

/**
 * @brief Called on a successfull TWR exchange, logs the measured distance on the
 *        device
 */
static void _twr_cb(twr_event_data_t *data, twr_status_t status)
{
    (void)data;
    (void)status;

    ed_t *ed = ed_list_process_rng_data(&_controller.ed_list, data->addr,
                                        pepper_sec_since_start(), data->range,
                                        data->los, data->rssi);

    (void)ed;

    if (LOG_LEVEL == LOG_DEBUG || IS_ACTIVE(CONFIG_PEPPER_LOG_UWB)) {
        ed_uwb_data_t uwb_data = {
#if IS_USED(MODULE_ED_UWB_RSSI)
            .rssi = data->rssi,
#endif
#if IS_USED(MODULE_ED_UWB_LOS)
            .los = data->los,
#endif
            .d_cm = data->range,
            .time = data->time,
            .cid = ed->cid,
        };

#if IS_USED(MODULE_PEPPER_SRV_STORAGE)
        pepper_srv_uwb_data_submit(&uwb_data);
#else
        if (IS_ACTIVE(CONFIG_PEPPER_LOG_CSV)) {
            ed_serialize_uwb_ble_printf_csv(&uwb_data, NULL, pepper_get_serializer_bn());
        }
        else {
            ed_serialize_uwb_printf(&uwb_data, pepper_get_serializer_bn());
        }
#endif
    }
}

/**
 * @brief Called when a TWR exchange timeouts
 */
static void _twr_timeout_cb(twr_event_data_t *data, twr_status_t status)
{
    (void)data;
    (void)status;
#if IS_USED(MODULE_ED_UWB_STATS)
    ed_t *ed = ed_list_get_by_short_addr(&_controller.ed_list, data->addr);
    if (status == TWR_RNG_INITIATOR) {
        LOG_DEBUG("[pepper]: req timeout 0x%04" PRIx16 "\n", data->addr);
        ed->uwb.stats.req.timeout++;
    }
    else {
        LOG_DEBUG("[pepper]: lst timeout 0x%04" PRIx16 "\n", data->addr);
        ed->uwb.stats.lst.timeout++;
    }
#endif
}

/**
 * @brief Called when a TWR exchange succeeds
 */
static void _twr_rx_cb(twr_event_data_t *data, twr_status_t status)
{
    (void)data;
    (void)status;
    ed_t *ed = ed_list_get_by_short_addr(&_controller.ed_list, data->addr);
    /* timestamp relative to beginning of epoch */
    uint32_t timestamp = pepper_sec_since_start();

    ed->uwb.seen_last_rx_s = timestamp;
}

static void _twr_busy_cb(twr_event_data_t *data, twr_status_t status)
{
    (void)data;
    (void)status;
#if IS_USED(MODULE_ED_UWB_STATS)
    ed_t *ed = ed_list_get_by_short_addr(&_controller.ed_list, data->addr);
    if (status == TWR_RNG_INITIATOR) {
        ed->uwb.stats.req.aborted++;
    }
    else {
        ed->uwb.stats.lst.aborted++;
    }
#endif
}
#endif

/**
 * @brief Called when valid PEPPER/DESIRE advertisements are scanned, this event
 *        is used to schedule TWR exchanges
 */
static void _scan_cb(uint32_t ticks, const ble_addr_t *addr, int8_t rssi,
                     const desire_ble_adv_payload_t *adv_payload)
{
    (void)addr;
    (void)ticks;
    (void)rssi;

    uint32_t cid;
    uint8_t part;
    /* timestamp relative to beginning of epoch */
    uint32_t timestamp = pepper_sec_since_start();
    /* seed for offset */
#if IS_USED(MODULE_TWR)
    uint16_t seed = adv_payload->data.reserved.seed;
#endif

    /* 1. process the incoming slice */
    decode_sid_cid(adv_payload->data.sid_cid, &part, &cid);
    ed_t *ed = ed_list_process_slice(&_controller.ed_list, cid, timestamp,
                                     adv_payload->data.ebid_slice, part);

    if (ed == NULL) {
        LOG_ERROR("return NULL\n");
        return;
    }
    /* 2. update last time this encounter was seen, relative to epoch start */
    ed->seen_last_s = timestamp;

    /* 3. if the EBID was reconstructed then either log BLE information (rssi)
          and/or scheduler a TWR exchange */
    if (ed->ebid.status.status == EBID_HAS_ALL) {
#if IS_USED(MODULE_ED_BLE) || IS_USED(MODULE_ED_BLE_WIN)
        /* 3.1 log rssi data */
        ed_list_process_scan_data(&_controller.ed_list, cid, timestamp, rssi);
#endif
#if IS_USED(MODULE_TWR)
        /* 3.2 check if should listen */
        if (_twr_should_listen(timestamp, ed)) {
#if IS_USED(MODULE_ED_UWB_STATS)
            ed->uwb.stats.lst.scheduled++;
#endif
            /* compensate for delay in scheduling listen */
            /* 3.3 schedule a twr listen event at an EBID based offset */
            uint16_t offset = _get_twr_rx_offset(&_controller.ebid, seed);
            if (twr_schedule_listen_managed(ed_get_short_addr(ed), offset)) {
#if IS_USED(MODULE_ED_UWB_STATS)
                ed->uwb.stats.lst.aborted++;
#endif
            }
            LOG_DEBUG("[pepper]: 0x04%" PRIx16 " rx offset %" PRIu16 "\n", ed_get_short_addr(
                          ed), offset);
        }
#endif
    }
#if IS_USED(MODULE_ED_BLE_COMMON)
    if (LOG_LEVEL == LOG_DEBUG || IS_ACTIVE(CONFIG_PEPPER_LOG_BLE)) {
        ed_ble_data_t ble_data = {
            .rssi = rssi,
            .time = ztimer_now(ZTIMER_MSEC),
            .cid = cid,
        };
#if IS_USED(MODULE_PEPPER_SRV_STORAGE)
        pepper_srv_ble_data_submit(&ble_data);
#else
        if (IS_ACTIVE(CONFIG_PEPPER_LOG_CSV)) {
            ed_serialize_uwb_ble_printf_csv(NULL, &ble_data, pepper_get_serializer_bn());
        }
        else {
            ed_serialize_ble_printf(&ble_data, pepper_get_serializer_bn());
        }
#endif
    }
#endif
}

/**
 * @brief Called when valid PEPPER/DESIRE advertisements are sent out, this event
 *        is used to schedule TWR exchanges
 */
static void _adv_cb(uint32_t advs, void *arg)
{
    (void)arg;
    (void)advs;

#if IS_USED(MODULE_TWR)
    ed_t *next = (ed_t *)_controller.ed_list.list.next;

    /* seed for offset */
    uint16_t seed = advs;

    /* timestamp relative to beginning of epoch */
    uint32_t timestamp = pepper_sec_since_start();
    /* system time in ticks */
    uint32_t now_ticks = ztimer_now(ZTIMER_MSEC_BASE);

    /* for all registered neighbors that have been seen over BLE recently schedule
       a TWR exchange request at an offset based on theire EBID */
    if (!next) {
        LOG_DEBUG("[pepper]: no neighbors\n");
        return;
    }
    do {
        next = (ed_t *)next->list_node.next;
        /* 1. check if it should send a request */
        if (_twr_should_request(timestamp, next)) {
            /* compensate for delay in scheduling requests */
            uint16_t delay = ztimer_now(ZTIMER_MSEC_BASE) - now_ticks;
            /* 2. schedule the request at the EBID based offset */
#if IS_USED(MODULE_ED_UWB_STATS)
            next->uwb.stats.req.scheduled++;
#endif
            uint16_t offset = _get_twr_tx_offset(&next->ebid, seed);
            if (twr_schedule_request_managed(ed_get_short_addr(next), offset - delay)) {
#if IS_USED(MODULE_ED_UWB_STATS)
                next->uwb.stats.req.aborted++;
#endif
            }
            LOG_DEBUG("[pepper]: 0x04%" PRIx16 " tx offset %" PRIu16 "\n", ed_get_short_addr(
                          next), offset);
            LOG_INFO("[pepper]: adv delay: %" PRIu16 ", offset: %" PRIu16 "\n",
                     delay, _get_twr_tx_offset(&next->ebid, seed));
        }
    } while (next != (ed_t *)_controller.ed_list.list.next);
#endif
}

void pepper_core_enable(ebid_t *ebid, ble_scan_params_t *scan_params,
                        adv_params_t *adv_params, uint32_t duration_ms)
{
    /* start advertising */
    LOG_DEBUG("[pepper]: start adv: %" PRIu32 " times with intervals of "
              "%" PRIu32 " ms\n", adv_params->advs_max, adv_params->itvl_ms);
    desire_ble_adv_start(ebid, adv_params);
#if IS_USED(MODULE_TWR)
    /* set new short addr */
    uint16_t short_addr = desire_ble_adv_get_cid();
    LOG_DEBUG("[pepper]: enable TWR with addr 0x%" PRIx16 "ms\n", short_addr);
    twr_enable();
    twr_set_short_addr(short_addr);
#endif
    LOG_DEBUG("[pepper]: start scanning for %" PRIu32 "ms\n", duration_ms);
    /* start scanning */
    desire_ble_scan_start(scan_params, duration_ms);
}

void pepper_core_disable(void)
{
    /* stop advertising and scanning */
    desire_ble_scan_stop();
    desire_ble_adv_stop();
#if IS_USED(MODULE_TWR)
    /* disable uwb */
    twr_disable();
#endif
}

static void _epoch_start(event_t *event)
{
    (void)event;
    /* timestamp the start of the epoch in relative units*/
    _controller.start_time = ztimer_now(ZTIMER_SEC);
    /* only use the ZTIMER_EPOCH timestamps for absolute and not for relative
       differences */
    LOG_INFO("[pepper]: new uwb_epoch t=%" PRIu32 "\n", ztimer_now(ZTIMER_EPOCH));
    epoch_init(&_controller.data, ztimer_now(ZTIMER_EPOCH), &_controller.keys);
    /* update local ebid */
    ebid_init(&_controller.ebid);
    LOG_INFO("[pepper]: new ebid generation\n");
    ebid_generate(&_controller.ebid, &_controller.keys);
    LOG_INFO("[pepper]: local ebid: \n\t");
    for (uint8_t i = 0; i < EBID_SIZE; i++) {
        if ((i + 1) % 8 == 0 && i != (EBID_SIZE - 1)) {
            LOG_INFO("0x%02x\n\t", _controller.ebid.parts.ebid.u8[i]);
        }
        else {
            LOG_INFO("0x%02x ", _controller.ebid.parts.ebid.u8[i]);
        }
    }
    LOG_INFO("\n");
    pepper_core_enable(&_controller.ebid, &_controller.scan, &_controller.adv,
                       _controller.epoch.duration_s * MS_PER_SEC);
}

static event_periodic_t _end_epoch;
static event_t _start_epoch = { .handler = _epoch_start };
static void _epoch_end(void *arg)
{
    (void)arg;
    /* first thing disable ble/uwb */
    pepper_core_disable();
    /* update controller status */
    mutex_lock(&_controller.lock);
    LOG_INFO("[pepper]: end of uwb_epoch\n");
    if (!ztimer_is_set(ZTIMER_EPOCH, &_end_epoch.timer.timer) && \
        _controller.status != PEPPER_PAUSED) {
        pepper_controller_set_status(PEPPER_STOPPED);
    }
    mutex_unlock(&_controller.lock);
    /* process uwb_epoch data */
    LOG_INFO("[pepper]: process all uwb_epoch data\n");
    ed_list_finish(&_controller.ed_list);
    epoch_finish(&_controller.data, &_controller.ed_list);
    /* post serializing/offloading event */
#if IS_USED(MODULE_PEPPER_SRV)
    pepper_srv_data_submit(&_controller.data);
#else
    contact_data_serialize_all_printf(&_controller.data, pepper_get_serializer_bn());
#endif
    /* bootstrap new epoch if required */
    if (pepper_is_active()) {
        /* by posting an event we allowed all other queued events to be handled */
        event_post(CONFIG_UWB_BLE_EVENT_PRIO, &_start_epoch);
    }
}
static event_callback_t _end_of_epoch = EVENT_CALLBACK_INIT(_epoch_end, NULL);

static void _align_end_of_epoch(uint32_t epoch_duration_s)
{
    /* ZTIMER_EPOCH is only used for absolute timestamps and aligning the end
       of epoch events */
    uint32_t timeout = epoch_duration_s - (ztimer_now(ZTIMER_EPOCH) % epoch_duration_s);

    /* schedule end of epoch event */
    event_periodic_set_count(&_end_epoch, _controller.epoch.iterations);
    event_periodic_start(&_end_epoch, timeout);
    /* fix subsequent timeouts */
    _end_epoch.timer.interval = epoch_duration_s;
    /* setup end of uwb_epoch timeout event */
    LOG_INFO("[pepper]: align first epoch end in %" PRIu32 "s\n", timeout);
}

void pepper_init(void)
{
    /* set initial status to STOPPED */
    pepper_controller_set_status(PEPPER_STOPPED);
    if (IS_USED(MODULE_PEPPER_UTIL)) {
        pepper_uid_init();
    }
    /* pepper_gatt and pepper_stdio_nimble are mutually exclusive */
    if (IS_USED(MODULE_PEPPER_GATT)) {
        pepper_gatt_init();
    }
    else if (IS_USED(MODULE_PEPPER_STDIO_NIMBLE)) {
        pepper_stdio_nimble_init();
    }
    if (IS_USED(MODULE_PEPPER_CURRENT_TIME)) {
        pepper_current_time_init();
    }
    /* init ble advertiser */
    desire_ble_adv_init(CONFIG_UWB_BLE_EVENT_PRIO);
    desire_ble_adv_set_cb(_adv_cb);
    /* init ble scanner and current_time */
    desire_ble_scan_init(_scan_cb);
    /* init twr */
#if IS_USED(MODULE_TWR)
    twr_event_mem_manager_init(&_controller.twr_mem);
    twr_managed_set_manager(&_controller.twr_mem);
    twr_init(CONFIG_UWB_BLE_EVENT_PRIO);
    twr_disable();
    twr_set_complete_cb(_twr_cb);
    twr_set_busy_cb(_twr_busy_cb);
    twr_set_rx_timeout_cb(_twr_timeout_cb);
    twr_set_rx_cb(_twr_rx_cb);
#endif
    /* init ed management */
    ed_memory_manager_init(&_controller.ed_mem);
    ed_list_init(&_controller.ed_list, &_controller.ed_mem, &_controller.ebid);
    /* setup end of uwb_epoch timeout event */
    event_periodic_init(&_end_epoch, ZTIMER_EPOCH, CONFIG_PEPPER_EVENT_PRIO,
                        &_end_of_epoch.super);
    /* set static cid if requested */
    if (IS_ACTIVE(CONFIG_BLE_ADV_STATIC_CID)) {
        uint32_t cid = 0x00000000;
        if (IS_USED(MODULE_PEPPER_UTIL)) {
            memcpy(&cid,
                   pepper_get_uid(),
                   PEPPER_UID_LEN > sizeof(uint32_t) ? sizeof(uint32_t) : PEPPER_UID_LEN);
            cid &= 0x3FFFFFFF;
        }
        else {
            cid = desire_ble_adv_gen_cid();
        }
        desire_ble_adv_set_cid(cid);
    }
}

void pepper_start(pepper_start_params_t *params)
{
    /* stop previous advertisements */
    pepper_stop();
    mutex_lock(&_controller.lock);
    /* clear encounter list */
    ed_list_clear(&_controller.ed_list);
    pepper_controller_set_status(PEPPER_RUNNING);
    /* set advertisement parameters */
    _controller.adv.itvl_ms = params->adv_itvl_ms;
    _controller.adv.advs_slice = params->advs_per_slice;
    _controller.adv.advs_max = (params->epoch_duration_s * MS_PER_SEC) / params->adv_itvl_ms;
    /* set epoch params */
    _controller.epoch.duration_s = params->epoch_duration_s;
    _controller.epoch.iterations = params->epoch_iterations;
    /* set scan params */
    _controller.scan.itvl_ms = params->scan_itvl_ms;
    _controller.scan.win_ms = params->scan_win_ms;
    /* set minimum duration */
    ed_list_set_min_exposure(&_controller.ed_list, _controller.epoch.duration_s / 3 );
    /* */
    /* align epoch start */
    if (params->align) {
        _align_end_of_epoch(_controller.epoch.duration_s);
    }
    else {
        /* schedule end of epoch event */
        event_periodic_set_count(&_end_epoch, _controller.epoch.iterations);
        event_periodic_start(&_end_epoch, _controller.epoch.duration_s);
    }
    mutex_unlock(&_controller.lock);
    /* bootstrap first epoch */
    event_post(CONFIG_UWB_BLE_EVENT_PRIO, &_start_epoch);
}

void pepper_resume(bool align)
{
    mutex_lock(&_controller.lock);
    if (pepper_is_active()) {
        /* align epoch start */
        if (align) {
            _align_end_of_epoch(_controller.epoch.duration_s);
        }
        else {
            /* schedule end of epoch event with remaining counts */
            event_periodic_set_count(&_end_epoch, _end_epoch.count);
            event_periodic_start(&_end_epoch, _controller.epoch.duration_s);
            /* re-enable BLE and UWB */
            pepper_core_enable(&_controller.ebid, &_controller.scan, &_controller.adv,
                               _controller.epoch.duration_s * MS_PER_SEC);
        }
        pepper_controller_set_status(PEPPER_RUNNING);
    }
    mutex_unlock(&_controller.lock);
}

void pepper_stop(void)
{
    mutex_lock(&_controller.lock);
    pepper_controller_set_status(PEPPER_STOPPED);
    event_periodic_stop(&_end_epoch);
    pepper_core_disable();
    mutex_unlock(&_controller.lock);
}

void pepper_pause(void)
{
    mutex_lock(&_controller.lock);
    pepper_core_disable();
    event_periodic_stop(&_end_epoch);
    if (pepper_is_active()) {
        pepper_controller_set_status(PEPPER_PAUSED);
    }
    else {
        pepper_controller_set_status(PEPPER_STOPPED);
    }
    mutex_unlock(&_controller.lock);
}

static void _set_status_led(uint8_t state)
{
    if (IS_USED(MODULE_PEPPER_STATUS_LED)) {
        if (gpio_is_valid(CONFIG_PEPPER_STATUS_LED)) {
            gpio_write(CONFIG_PEPPER_STATUS_LED, state);
        }
    }
}

controller_status_t pepper_controller_get_status(void)
{
    return _controller.status;
}

void pepper_controller_set_status(controller_status_t status)
{
    _controller.status = status;
    if (_controller.status != PEPPER_STOPPED) {
        _set_status_led(0);
    }
    else {
        _set_status_led(1);
    }
}

bool pepper_is_active(void)
{
    return _controller.status != PEPPER_STOPPED;
}

void pepper_twr_set_rx_offset(int16_t ticks)
{
#if IS_USED(MODULE_TWR)
    assert(ticks > -1 * (int16_t)CONFIG_TWR_MIN_OFFSET_TICKS);
    _controller.twr_params.rx_offset_ticks = ticks;
#else
    (void)ticks;
#endif
}

void pepper_twr_set_tx_offset(int16_t ticks)
{
#if IS_USED(MODULE_TWR)
    assert(ticks > (int16_t)-1 * (int16_t)CONFIG_TWR_MIN_OFFSET_TICKS);
    _controller.twr_params.tx_offset_ticks = ticks;
#else
    (void)ticks;
#endif
}

int16_t pepper_twr_get_rx_offset(void)
{
#if IS_USED(MODULE_TWR)
    return _controller.twr_params.rx_offset_ticks;
#else
    return 0;
#endif
}

int16_t pepper_twr_get_tx_offset(void)
{
#if IS_USED(MODULE_TWR)
    return _controller.twr_params.tx_offset_ticks;
#else
    return 0;
#endif
}

uint16_t pepper_twr_get_backoff(void)
{
#if IS_USED(MODULE_TWR)
    return _controller.twr_params.backoff;
#else
    return 0;
#endif
}

void pepper_twr_set_backoff(uint16_t backoff)
{
    (void)backoff;
#if IS_USED(MODULE_TWR)
    _controller.twr_params.backoff = backoff;
#endif
}

controller_t *pepper_get_controller(void)
{
    return &_controller;
}
