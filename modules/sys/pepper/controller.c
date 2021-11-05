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

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

typedef struct controller {
    ebid_t ebid;                        /**> */
    ed_list_t ed_list;                  /**> */
    ed_memory_manager_t ed_mem;         /**> */
    twr_event_mem_manager_t twr_mem;    /**> */
    crypto_manager_keys_t keys;         /**> */
    uint32_t start_time;                /**> */
    epoch_data_t data;                  /**> */
    epoch_data_t data_serialize;        /**> */
} controller_t;
static controller_t _controller;

static adv_params_t _adv_params;
static twr_params_t _twr_params = {
    .rx_offset_ticks = CONFIG_TWR_RX_OFFSET_TICKS,
    .tx_offset_ticks = CONFIG_TWR_TX_OFFSET_TICKS
};

static char _base_name[CONFIG_PEPPER_BASE_NAME_BUFFER] = "pepper";

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
    return _get_twr_offset(ebid) + _twr_params.rx_offset_ticks;
}

static uint16_t _get_twr_tx_offset(ebid_t *ebid)
{
    return _get_twr_offset(ebid) + _twr_params.tx_offset_ticks;
}

static void _twr_cb(twr_event_data_t *data)
{
    (void)data;
    /* get relative event timestamp */
    /* TODO: add an API for this */
    uint32_t timestamp = ztimer_now(ZTIMER_SEC) - _controller.start_time;

    ed_t* ed = ed_list_process_rng_data(&_controller.ed_list, data->addr, timestamp, data->range);

    if (LOG_LEVEL == LOG_INFO) {
        /* TODO: move to stop watch api */
        ed_serialize_uwb_json(data->range, ed->cid, ztimer_now(ZTIMER_EPOCH), _base_name);
    }
    if(LOG_LEVEL == LOG_DEBUG) {
        print_str("[ble/uwb] twr: addr=(0x");
        print_byte_hex(data->addr >> 8);
        print_byte_hex(data->addr);
        print_str("), d=(");
        print_u32_dec(data->range);
        print_str("cm)\n");
    }
}

static void _scan_cb(uint32_t ticks, const ble_addr_t *addr, int8_t rssi,
                     const desire_ble_adv_payload_t *adv_payload)
{
    (void)addr;
    (void)ticks;
    (void)rssi;

    uint32_t cid;
    uint8_t part;
    /* get relative event timestamp */
    /* TODO: add an API for this */
    uint32_t timestamp = ztimer_now(ZTIMER_SEC) - _controller.start_time;

    /* process the incoming slice */
    decode_sid_cid(adv_payload->data.sid_cid, &part, &cid);
    ed_t *ed = ed_list_process_slice(&_controller.ed_list, cid, timestamp,
                                     adv_payload->data.ebid_slice, part);

    if (ed->ebid.status.status == EBID_HAS_ALL) {
#if IS_USED(MODULE_ED_BLE) || IS_USED(MODULE_ED_BLE_WIN)
        /* log rssi data */
        ed_list_process_scan_data(&_controller.ed_list, cid, timestamp, rssi);
#endif
        /* schedule a twr listen event at an EBID based offset */
        twr_schedule_listen_managed(_get_twr_rx_offset(&_controller.ebid));
    }

    if (LOG_LEVEL == LOG_INFO) {
        /* TODO: move to stop watch api */
        ed_serialize_ble_json(rssi, cid, ztimer_now(ZTIMER_EPOCH), _base_name);
    }
}

static void _adv_cb(uint32_t advs, void *arg)
{
    (void)arg;
    (void)advs;

    ed_t *next = (ed_t *)_controller.ed_list.list.next;

    /* get relative event timestamp */
    /* TODO: add an API for this */
    uint32_t timestamp = ztimer_now(ZTIMER_SEC) - _controller.start_time;
    /* system time in ms */
    uint32_t now = ztimer_now(ZTIMER_MSEC);
    /* system time in ticks */
    uint32_t now_ticks = ztimer_now(ZTIMER_MSEC_BASE);

    if (!next) {
        LOG_DEBUG("[ble/uwb]: no neighbors\n");
        return;
    }
    do {
        next = (ed_t *)next->list_node.next;
        if (next->ebid.status.status == EBID_HAS_ALL) {
            /* check if the neighbor was also seen over BLE in the last CONFIG_MIA_TIME_S */
            if (next->uwb.seen_last_s > timestamp - CONFIG_MIA_TIME_S) {
                /* schedule the request at offset corrected by the delay */
                uint16_t delay = ztimer_now(ZTIMER_MSEC_BASE) - now_ticks;
                twr_schedule_request_managed((uint16_t)next->cid,
                                             _get_twr_tx_offset(&next->ebid) - delay);
            }
            else {
                LOG_DEBUG("[ble/uwb]: skip encounter, missing over BLE\n");
            }
        }
    } while (next != (ed_t *)_controller.ed_list.list.next);
    LOG_DEBUG("[ble/uwb] %" PRIu32 ": adv_cb\n", now);
}

static void _epoch_setup(void *arg)
{
    (void)arg;
    /* timestamp the start of the epoch in relative units*/
    _controller.start_time = ztimer_now(ZTIMER_SEC);
    /* timestamp absolute */
    uint32_t now_epoch = ztimer_now(ZTIMER_EPOCH);

    LOG_INFO("[pepper]: new uwb_epoch t=%" PRIu32 "\n", now_epoch);
    /* initiate epoch and generate new keys */
    epoch_init(&_controller.data, now_epoch, &_controller.keys);
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

static void _epoch_align(uint32_t epoch_duration_s)
{
    /* setup end of uwb_epoch timeout event */
    uint32_t modulo = ztimer_now(ZTIMER_EPOCH) % epoch_duration_s;
    uint32_t diff = modulo ? epoch_duration_s - modulo : 0;
    LOG_INFO("[pepper]: delay for %" PRIu32 "s to align uwb_epoch start\n",
             diff);
    ztimer_sleep(ZTIMER_EPOCH, diff);
}

static void _epoch_start(void *arg)
{
    (void)arg;
    /* start advertisement */
    LOG_INFO("[pepper]: start adv: %" PRIu32 " times with intervals of "
             "%" PRIu32 " ms\n", _adv_params.max_events, _adv_params.itvl_ms);
    desire_ble_adv_start(&_controller.ebid, _adv_params.itvl_ms,
                         _adv_params.max_events, _adv_params.max_events_slice);
    /* set new short addr */
    twr_set_short_addr(desire_ble_adv_get_cid());
    /* enable twr activity */
    twr_enable();
    /* start scanning */
    uint32_t scan_duration_ms = _adv_params.itvl_ms * _adv_params.max_events;
    LOG_INFO("[pepper]: start scanning for %" PRIu32 "ms\n", scan_duration_ms);
    desire_ble_scan_start(scan_duration_ms);
}

typedef struct {
    event_t super;
    epoch_data_t *data;
} epoch_data_event_t;
static void _serialize_epoch_handler(event_t *event)
{
    epoch_data_event_t *d_event = (epoch_data_event_t *)event;

    LOG_INFO("[pepper]: dumping epoch data\n");
    contact_data_serialize_all_printf(d_event->data, _base_name);
}
static epoch_data_event_t _serialize_epoch =
{ .super.handler = _serialize_epoch_handler };

static void _epoch_end(void *arg)
{
    (void)arg;
    LOG_INFO("[pepper]: end of uwb_epoch\n");
    /* stop advertising and scanning */
    desire_ble_scan_stop();
    desire_ble_adv_stop();
    /* process uwb_epoch data */
    LOG_INFO("[pepper]: process all uwb_epoch data\n");
    ed_list_finish(&_controller.ed_list);
    epoch_finish(&_controller.data, &_controller.ed_list);
    /* post serializing/offloading event */
    memcpy(&_controller.data_serialize, &_controller.data, sizeof(epoch_data_t));
    _serialize_epoch.data = &_controller.data_serialize;
    /* bootstrap new uwb_epoch */
    _epoch_setup(NULL);
    _epoch_start(NULL);
    /* post after bootstrapping the new epoch */
    event_post(EVENT_PRIO_MEDIUM, &_serialize_epoch.super);
}
static event_periodic_t _end_epoch;
static event_callback_t _end_of_epoch = EVENT_CALLBACK_INIT(_epoch_end, NULL);

void pepper_init(void)
{
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
}

void pepper_start(uint32_t epoch_duration_s, uint32_t advs_per_slice,
                  uint32_t adv_itvl_ms, bool align)
{
    /* stop previous advertisements */
    pepper_stop();
    /* setup end of uwb_epoch timeout event */
    event_periodic_init(&_end_epoch, ZTIMER_EPOCH, EVENT_PRIO_HIGHEST,
                        &_end_of_epoch.super);
    /* set advertisement parameters */
    _adv_params.itvl_ms = adv_itvl_ms;
    _adv_params.max_events_slice = advs_per_slice;
    _adv_params.max_events = (epoch_duration_s * MS_PER_SEC) / adv_itvl_ms;
    /* set minimum duration */
    ed_list_set_min_exposure(&_controller.ed_list, epoch_duration_s / 3 );
    /* align epoch start */
    if (align) {
        _epoch_align(epoch_duration_s);
    }
    /* schedule end of epoch event */
    event_periodic_start(&_end_epoch, epoch_duration_s);
    /* bootstrap first epoch */
    _epoch_setup(NULL);
    _epoch_start(NULL);
}

void pepper_stop(void)
{
    event_periodic_stop(&_end_epoch);
    desire_ble_adv_stop();
    desire_ble_scan_stop();
    twr_disable();
}

int pepper_pause(void)
{
    int ret = 0;
    if (ztimer_is_set(ZTIMER_EPOCH, &_end_epoch.timer)) {
        ret = 1;
    }
    pepper_stop();
    return ret;
}

void pepper_twr_set_rx_offset(int16_t ticks)
{
    assert(ticks > -1 * (int16_t)CONFIG_TWR_MIN_OFFSET_TICKS);
    _twr_params.rx_offset_ticks = ticks;
}

void pepper_twr_set_tx_offset(int16_t ticks)
{
    assert(ticks > (int16_t)-1 * (int16_t)CONFIG_TWR_MIN_OFFSET_TICKS);
    _twr_params.tx_offset_ticks = ticks;
}

int16_t pepper_twr_get_rx_offset(void)
{
    return _twr_params.rx_offset_ticks;
}

int16_t pepper_twr_get_tx_offset(void)
{
    return _twr_params.tx_offset_ticks;
}

int pepper_set_serializer_base_name(char* base_name)
{
    if (strlen(base_name) > CONFIG_PEPPER_BASE_NAME_BUFFER) {
        return -1;
    }
    memcpy(_base_name, base_name, strlen(base_name));
    _base_name[strlen(base_name)] = '\0';
    return 0;
}
