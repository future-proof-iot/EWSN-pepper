/*
 * Copyright (C) 2022 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_ble_scanner
 * @{
 *
 * @file
 * @brief       NimBLE Scanner Multiplexing Module implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include "ble_scanner.h"
#include "nimble_scanner.h"
#include "ztimer.h"

#include "net/bluetil/ad.h"
#include "net/bluetil/addr.h"
#include "host/ble_hs.h"
#include "nimble/hci_common.h"

#include "ble_pkt_dbg.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

static ble_scan_listener_t _list;
static bool _enabled;

/* just a convenience type to pass all data as a struct reference to
   the clist_foreach function */
typedef struct listener_cb_arg {
    uint8_t adv_type;
    const ble_addr_t *addr;
    uint32_t ts;
    const nimble_scanner_info_t *info;
    const bluetil_ad_t *ad;
} listener_cb_arg_t;

static int _listener_cb(clist_node_t *node, void *arg)
{
    ble_scan_listener_t *listener = (ble_scan_listener_t *)node;
    listener_cb_arg_t *cb_arg = (listener_cb_arg_t *)arg;
    listener->cb(cb_arg->adv_type, cb_arg->addr, cb_arg->ts, cb_arg->info,
                 cb_arg->ad, listener->arg);
    return 0;
}

void ble_scanner_register(ble_scan_listener_t *listener)
{
    unsigned state = irq_disable();

    if (!listener->list_node.next) {
        LOG_INFO("[ble_scanner]: register listener\n");
        clist_rpush(&_list.list_node, &listener->list_node);
    }
    irq_restore(state);
    if (_enabled && ble_gap_disc_active()) {
        LOG_INFO("[ble_scanner]: new listener re-start\n");
        nimble_scanner_start();
    }
}

void ble_scanner_unregister(ble_scan_listener_t *listener)
{
    unsigned state = irq_disable();

    LOG_INFO("[ble_scanner]: unregister unregister\n");
    clist_remove(&_list.list_node, &listener->list_node);
    listener->list_node.next = NULL;
    irq_restore(state);
    if (clist_count(&_list.list_node) == 0) {
        LOG_INFO("[ble_scanner]: no listeners stop\n");
        nimble_scanner_stop();
    }
}

static void _on_scan_evt(uint8_t type, const ble_addr_t *addr,
                         const nimble_scanner_info_t *info,
                         const uint8_t *adv, size_t adv_len)
{
    assert(addr);
    assert(adv_len <= BLE_ADV_PDU_LEN);

    bluetil_ad_t ad = BLUETIL_AD_INIT((uint8_t *)adv, adv_len, adv_len);
    uint32_t ts = ztimer_now(ZTIMER_MSEC);

    if (LOG_LEVEL == LOG_DEBUG) {
        LOG_DEBUG("[ble_scanner]:\n");
        dbg_print_ble_addr(addr);
        dbg_dump_buffer("\t adv_pkt = ", adv, adv_len, '\n');
    }

    listener_cb_arg_t arg = {
        .ad = &ad,
        .addr = addr,
        .adv_type = type,
        .info = info,
        .ts = ts,
    };

    clist_foreach(&_list.list_node, _listener_cb, &arg);
}

void ble_scanner_update(const ble_scan_params_t *params)
{
    nimble_scanner_cfg_t scan_params;

    if (_enabled) {
        nimble_scanner_stop();
    }

    LOG_INFO("[ble_scanner]: update scan parmeters\n");
    scan_params.itvl_ms = params->itvl_ms;
    scan_params.win_ms = params->win_ms;
    scan_params.flags = NIMBLE_SCANNER_PHY_1M;

    int ret = nimble_scanner_init(&scan_params, _on_scan_evt);

    if (_enabled) {
        nimble_scanner_start();
    }

    assert(ret == 0);
    (void)ret;
}

void ble_scanner_start(int32_t scan_duration_ms)
{
    _enabled = true;

    LOG_INFO("[ble_scanner]: start for ");
    if (scan_duration_ms == BLE_HS_FOREVER) {
        LOG_INFO("forever\n");
    }
    else {
        LOG_INFO("%" PRIi32 "\n", scan_duration_ms);
    }
    /* stop previous ongoing scan if any */
    nimble_scanner_stop();
    /* set scan duration */
    nimble_scanner_set_scan_duration(scan_duration_ms);
    /* start scanning */
    int ret = nimble_scanner_start();

    assert(ret == 0);
    (void)ret;
}

void ble_scanner_stop(void)
{
    LOG_INFO("[ble_scanner]: stop\n");
    if (_enabled) {
        nimble_scanner_stop();
    }
    _enabled = false;

    nimble_scanner_stop();
}

void ble_scanner_init(const ble_scan_params_t *params)
{
    ble_scanner_update(params);

    if (IS_ACTIVE(CONFIG_BLE_SCANNER_AUTO_START)) {
        ble_scanner_start(BLE_HS_FOREVER);
    }
}

bool ble_scanner_is_enabled(void)
{
    return _enabled;
}
