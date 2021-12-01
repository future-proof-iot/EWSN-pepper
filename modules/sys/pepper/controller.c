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
#include "twr.h"
#include "ebid.h"
#include "desire_ble_adv.h"
#include "desire_ble_scan.h"
#include "desire_ble_scan_params.h"
#if IS_USED(MODULE_STORAGE)
#include "storage.h"
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

static controller_t _controller = {
    .lock = MUTEX_INIT,
    .twr_params = {
        .rx_offset_ticks = CONFIG_TWR_RX_OFFSET_TICKS,
        .tx_offset_ticks = CONFIG_TWR_TX_OFFSET_TICKS
    }
};

static uint16_t _get_twr_offset(ebid_t *ebid)
{
    /* last two bytes of the EBID */
    uint16_t offset_ms = ebid->parts.ebid.u8[0] + (ebid->parts.ebid.u8[1] << 8);

    /* add a minimum offset and a random EBID based one */
    offset_ms = (offset_ms % CONFIG_BLE_ADV_INTERVAL_MS) + CONFIG_TWR_MIN_OFFSET_MS;
    /* TODO: add some RIOT functions for this */
    return os_cputime_usecs_to_ticks(offset_ms * US_PER_MS);
}

static uint16_t _get_twr_rx_offset(ebid_t *ebid)
{
    return _get_twr_offset(ebid) + _controller.twr_params.rx_offset_ticks;
}

static uint16_t _get_twr_tx_offset(ebid_t *ebid)
{
    return _get_twr_offset(ebid) + _controller.twr_params.tx_offset_ticks;
}

static uint32_t pepper_sec_since_start(void)
{
    return ztimer_now(ZTIMER_SEC) - _controller.start_time;
}

/**
 * @brief Called on a successfull TWR exchange, logs the measure distance on the
 *        device
 */
static void _twr_cb(twr_event_data_t *data)
{
    (void)data;

    ed_t *ed = ed_list_process_rng_data(&_controller.ed_list, data->addr,
                                        pepper_sec_since_start(), data->range,
                                        data->los);

    (void)ed;

    if (LOG_LEVEL == LOG_INFO) {
        /* log with an absolute epoch based timestamp */
        ed_serialize_uwb_json(data->range, data->los, ed->cid, ztimer_now(ZTIMER_EPOCH),
                              pepper_get_serializer_bn());
    }
}

/**
 * @brief Called when valid PEPPER/DESIRE advertisements are scanned, this event
 *        is used to scheduler TWR exchanges
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

    /* 1. process the incoming slice */
    decode_sid_cid(adv_payload->data.sid_cid, &part, &cid);
    ed_t *ed = ed_list_process_slice(&_controller.ed_list, cid, timestamp,
                                     adv_payload->data.ebid_slice, part);

    if (ed == NULL) {
        LOG_ERROR("return NULL");
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
        /* 3.2 schedule a twr listen event at an EBID based offset */
        twr_schedule_listen_managed(_get_twr_rx_offset(&_controller.ebid));
#endif
        if (LOG_LEVEL == LOG_DEBUG) {
#if IS_USED(MODULE_ED_BLE) || IS_USED(MODULE_ED_BLE_WIN)
            /* log with an absolute epoch based timestamp */
            ed_serialize_ble_json(rssi, cid, ztimer_now(ZTIMER_EPOCH),
                                  pepper_get_serializer_bn());
#endif
        }
    }
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

    /* timestamp relative to beginning of epoch */
    uint32_t timestamp = pepper_sec_since_start();
    /* system time in ticks */
    uint32_t now_ticks = ztimer_now(ZTIMER_MSEC_BASE);

    /* for all registered neighbors that have been seen over BLE recently schedule
       a TWR exchange request at an offset based on theire EBID */
    if (!next) {
        LOG_DEBUG("[ble/uwb]: no neighbors\n");
        return;
    }
    do {
        next = (ed_t *)next->list_node.next;
        if (next->ebid.status.status == EBID_HAS_ALL) {
            /* 1. check if advertisement where received from neighbor in last CONFIG_MIA_TIME_S */
            if (next->seen_last_s > timestamp - CONFIG_MIA_TIME_S) {
                /* compensate for delay in scheduling requests */
                uint16_t delay = ztimer_now(ZTIMER_MSEC_BASE) - now_ticks;
                /* 2. schedule the request at the EBID based offset */
                twr_schedule_request_managed(ed_get_short_addr(next),
                                             _get_twr_tx_offset(&next->ebid) - delay);
            }
            else {
                LOG_DEBUG("[ble/uwb]: skip encounter, missing over BLE\n");
            }
        }
    } while (next != (ed_t *)_controller.ed_list.list.next);
#endif
}

void pepper_core_enable(ebid_t *ebid, adv_params_t *params, uint32_t duration_ms)
{
    /* start advertising */
    LOG_DEBUG("[pepper]: start adv: %" PRIu32 " times with intervals of "
              "%" PRIu32 " ms\n", params->advs_max, params->itvl_ms);
    desire_ble_adv_start(ebid, params);
#if IS_USED(MODULE_TWR)
    /* set new short addr */
    uint16_t short_addr = desire_ble_adv_get_cid();
    LOG_DEBUG("[pepper]: enable TWR with addr 0x%" PRIx16 "ms\n", short_addr);
    twr_set_short_addr(short_addr);
    twr_enable();
#endif
    LOG_DEBUG("[pepper]: start scanning for %" PRIu32 "ms\n", duration_ms);
    /* start scanning */
    desire_ble_scan_start(duration_ms);
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

static void _epoch_setup(void *arg)
{
    (void)arg;
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
    LOG_INFO("[pepper]: local ebid: [");
    for (uint8_t i = 0; i < EBID_SIZE; i++) {
        LOG_INFO("%d, ", _controller.ebid.parts.ebid.u8[i]);
    }
    LOG_INFO("]\n");
}

static void _epoch_start(void *arg)
{
    (void)arg;
    pepper_core_enable(&_controller.ebid, &_controller.adv,
                       _controller.epoch.duration_s * MS_PER_SEC);
}

typedef struct {
    event_t super;
    epoch_data_t *data;
} epoch_data_event_t;
static uint8_t sd_buffer[IS_ACTIVE(MODULE_STORAGE) * 2048];
static void _serialize_epoch_handler(event_t *event)
{
    epoch_data_event_t *d_event = (epoch_data_event_t *)event;

    LOG_INFO("[pepper]: dumping epoch data\n");
    if (IS_USED(MODULE_STORAGE)) {
        size_t len = contact_data_serialize_all_json(d_event->data,
                                                     sd_buffer, sizeof(sd_buffer),
                                                     pepper_get_serializer_bn());
        (void)len;
#if IS_USED(MODULE_STORAGE)
        storage_log(CONFIG_PEPPER_LOGFILE, sd_buffer, len - 1 );
#endif
    }
    if (!IS_USED(MODULE_PEPPER_STDIO_NIMBLE) || !IS_USED(MODULE_STORAGE)) {
        contact_data_serialize_all_printf(d_event->data, pepper_get_serializer_bn());
    }
}

static epoch_data_event_t _serialize_epoch = { .super.handler = _serialize_epoch_handler };

static event_periodic_t _end_epoch;
static void _epoch_end(void *arg)
{
    (void)arg;
    /* update controller status */
    mutex_lock(&_controller.lock);
    LOG_INFO("[pepper]: end of uwb_epoch\n");
    if (!ztimer_is_set(ZTIMER_EPOCH, &_end_epoch.timer.timer) && \
        _controller.status != PEPPER_PAUSED) {
        _controller.status = PEPPER_STOPPED;
        LED3_OFF;
    }
    mutex_unlock(&_controller.lock);
    pepper_core_disable();
    /* process uwb_epoch data */
    LOG_INFO("[pepper]: process all uwb_epoch data\n");
    ed_list_finish(&_controller.ed_list);
    epoch_finish(&_controller.data, &_controller.ed_list);
    /* post serializing/offloading event */
    memcpy(&_controller.data_serialize, &_controller.data, sizeof(epoch_data_t));
    _serialize_epoch.data = &_controller.data_serialize;
    event_post(EVENT_PRIO_MEDIUM, &_serialize_epoch.super);
    /* bootstrap new epoch if required */
    if (pepper_is_active()) {
        _epoch_setup(NULL);
        _epoch_start(NULL);
    }
}
static event_callback_t _end_of_epoch = EVENT_CALLBACK_INIT(_epoch_end, NULL);

static void _align_end_of_epoch(uint32_t epoch_duration_s)
{
    /* ZTIMER_EPOCH is only used for absolute timestamps and aligning the end
       of epoch events */
    uint32_t timeout = epoch_duration_s - (ztimer_now(ZTIMER_EPOCH) % epoch_duration_s);

    /* schedule end of epoch event */
    event_periodic_start_iter(&_end_epoch, timeout, _controller.epoch.iterations);
    /* fix subsequent timeouts */
    _end_epoch.timer.interval = epoch_duration_s;
    /* setup end of uwb_epoch timeout event */
    LOG_INFO("[pepper]: align first epoch end in %" PRIu32 "s\n", timeout);
}

void pepper_init(void)
{
    /* set initial status to STOPPED */
    _controller.status = PEPPER_STOPPED;
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
#if IS_USED(MODULE_STORAGE)
    storage_init();
#endif
    /* init ble advertiser */
    desire_ble_adv_init(EVENT_PRIO_HIGHEST);
    desire_ble_adv_set_cb(_adv_cb);
    /* init ble scanner and current_time */
    desire_ble_scan_init(&desire_ble_scanner_params, _scan_cb);
    /* init twr */
    twr_event_mem_manager_init(&_controller.twr_mem);
    twr_managed_set_manager(&_controller.twr_mem);
    twr_init(EVENT_PRIO_HIGHEST);
    twr_disable();
    twr_register_rng_cb(_twr_cb);
    /* init ed management */
    ed_memory_manager_init(&_controller.ed_mem);
    ed_list_init(&_controller.ed_list, &_controller.ed_mem, &_controller.ebid);
    /* setup end of uwb_epoch timeout event */
    event_periodic_init(&_end_epoch, ZTIMER_EPOCH, EVENT_PRIO_HIGHEST, &_end_of_epoch.super);
}

void pepper_start(pepper_start_params_t *params)
{
    /* stop previous advertisements */
    pepper_stop();
    mutex_lock(&_controller.lock);
    _controller.status = PEPPER_RUNNING;
    /* set advertisement parameters */
    _controller.adv.itvl_ms = params->adv_itvl_ms;
    _controller.adv.advs_slice = params->advs_per_slice;
    _controller.adv.advs_max = (params->epoch_duration_s * MS_PER_SEC) / params->adv_itvl_ms;
    /* set epoch params */
    _controller.epoch.duration_s = params->epoch_duration_s;
    _controller.epoch.iterations = params->epoch_iterations;
    /* set minimum duration */
    ed_list_set_min_exposure(&_controller.ed_list, _controller.epoch.duration_s / 3 );
    /* */
    /* align epoch start */
    if (params->align) {
        _align_end_of_epoch(_controller.epoch.duration_s);
    }
    else {
        /* schedule end of epoch event */
        event_periodic_start_iter(&_end_epoch, _controller.epoch.duration_s,
                                  _controller.epoch.iterations);
    }
    LED3_ON;
    mutex_unlock(&_controller.lock);
    /* bootstrap first epoch */
    _epoch_setup(NULL);
    _epoch_start(NULL);
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
            event_periodic_start_iter(&_end_epoch, _controller.epoch.duration_s,
                                      _end_epoch.count);
            /* re-enable BLE and UWB */
            pepper_core_enable(&_controller.ebid, &_controller.adv,
                               _controller.epoch.duration_s);
        }
        _controller.status = PEPPER_RUNNING;
    }
    mutex_unlock(&_controller.lock);
}

void pepper_stop(void)
{
    mutex_lock(&_controller.lock);
    LED3_OFF;
    _controller.status = PEPPER_STOPPED;
    event_periodic_stop(&_end_epoch);
    pepper_core_disable();
    ed_list_clear(&_controller.ed_list);
    mutex_unlock(&_controller.lock);
}

void pepper_pause(void)
{
    mutex_lock(&_controller.lock);
    pepper_core_disable();
    event_periodic_stop(&_end_epoch);
    if (pepper_is_active()) {
        _controller.status = PEPPER_PAUSED;
    }
    else {
        _controller.status = PEPPER_STOPPED;
    }
    mutex_unlock(&_controller.lock);
}

bool pepper_is_active(void)
{
    return _controller.status != PEPPER_STOPPED;
}

void pepper_twr_set_rx_offset(int16_t ticks)
{
    assert(ticks > -1 * (int16_t)CONFIG_TWR_MIN_OFFSET_TICKS);
    _controller.twr_params.rx_offset_ticks = ticks;
}

void pepper_twr_set_tx_offset(int16_t ticks)
{
    assert(ticks > (int16_t)-1 * (int16_t)CONFIG_TWR_MIN_OFFSET_TICKS);
    _controller.twr_params.tx_offset_ticks = ticks;
}

int16_t pepper_twr_get_rx_offset(void)
{
    return _controller.twr_params.rx_offset_ticks;
}

int16_t pepper_twr_get_tx_offset(void)
{
    return _controller.twr_params.tx_offset_ticks;
}

controller_t *pepper_get_controller(void)
{
    return &_controller;
}
