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
 * @author      Anonymous
 *
 * @}
 */

#include "ble_scanner_netif_params.h"
#include "ble_scanner_params.h"
#include "ble_scanner.h"

#include "net/bluetil/ad.h"
#include "net/bluetil/addr.h"
#include "host/ble_hs.h"
#include "nimble/hci_common.h"

#include "nimble_riot.h"
#include "nimble_netif.h"
#include "nimble_netif_conn.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

enum {
    STATE_IDLE,
    STATE_CONNECTED,
    STATE_CONNECTING,
    STATE_CLOSED,
};

static volatile uint8_t _state = STATE_IDLE;
static nimble_netif_connect_cfg_t _conn_params;

static void _evt_dbg(const char *msg, int handle, const uint8_t *addr)
{
    if (LOG_LEVEL == LOG_DEBUG) {
        printf("[ble_scanner] netif %s (%i|", msg, handle);
        if (addr) {
            bluetil_addr_print(addr);
        }
        else {
            printf("n/a");
        }
        puts(")");
    }
}

static void _on_netif_evt(int handle, nimble_netif_event_t event,
                          const uint8_t *addr)
{
    switch (event) {
    case NIMBLE_NETIF_ACCEPTING:
        _evt_dbg("ACCEPTING", handle, addr);
        break;
    case NIMBLE_NETIF_ACCEPT_STOP:
        _evt_dbg("ACCEPT_STOP", handle, addr);
        break;
    case NIMBLE_NETIF_INIT_MASTER:
        _evt_dbg("CONN_INIT master", handle, addr);
        _state = STATE_CONNECTING;
        break;
    case NIMBLE_NETIF_INIT_SLAVE:
        _evt_dbg("CONN_INIT slave", handle, addr);
        _state = STATE_CONNECTING;
        break;
    case NIMBLE_NETIF_CONNECTED_MASTER:
        _evt_dbg("CONNECTED master", handle, addr);
        _state = STATE_CONNECTED;
        break;
    case NIMBLE_NETIF_CONNECTED_SLAVE:
        _evt_dbg("CONNECTED slave", handle, addr);
        _state = STATE_CONNECTED;
        break;
    case NIMBLE_NETIF_CLOSED_MASTER:
        _evt_dbg("CLOSED master", handle, addr);
        _state = STATE_IDLE;
        break;
    case NIMBLE_NETIF_CLOSED_SLAVE:
        _evt_dbg("CLOSED slave", handle, addr);
        _state = STATE_CLOSED;
        break;
    case NIMBLE_NETIF_ABORT_MASTER:
        _evt_dbg("ABORT master", handle, addr);
        _state = STATE_IDLE;
        break;
    case NIMBLE_NETIF_ABORT_SLAVE:
        _evt_dbg("ABORT slave", handle, addr);
        _state = STATE_IDLE;
        break;
    case NIMBLE_NETIF_CONN_UPDATED:
        _evt_dbg("UPDATED", handle, addr);
        break;
    default:
        /* this should never happen */
        assert(0);
    }

    if (ble_scanner_is_enabled() && _state != STATE_CONNECTING) {
        LOG_DEBUG("[ble_scanner] netif: restart scanner\n");
        nimble_scanner_start();
    }
}

static int _conn_update(nimble_netif_conn_t *conn, int handle, void *arg)
{
    (void)conn;
    (void)arg;
    nimble_netif_update(handle, (const struct ble_gap_upd_params *)arg);
    return 0;
}

static int _filter_uuid(const bluetil_ad_t *ad)
{
    bluetil_ad_data_t incomp;

    if (bluetil_ad_find(ad, BLE_GAP_AD_UUID16_INCOMP,
                        &incomp) == BLUETIL_AD_OK) {
        uint16_t filter_uuid = BLE_GATT_SVC_IPSS;
        for (unsigned i = 0; i < incomp.len; i += 2) {
            if (memcmp(&filter_uuid, &incomp.data[i], 2) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

void _netif_cb(uint8_t adv_type, const ble_addr_t *addr,
               uint32_t ts, const nimble_scanner_info_t *info,
               const bluetil_ad_t *ad, void *arg)
{
    (void)info;
    (void)arg;
    (void)ts;

    /* only interested in NONCONN adv */
    if (adv_type == BLE_HCI_ADV_TYPE_ADV_IND) {
        /* for connection checking we need the address in network byte order */
        uint8_t addrn[BLE_ADDR_LEN];

        bluetil_addr_swapped_cp(addr->val, addrn);

        if (_filter_uuid(ad) && !nimble_netif_conn_connected(addrn)) {
            LOG_DEBUG("[ble_scanner] netif: stop scanner\n");
            nimble_scanner_stop();
            LOG_DEBUG("[ble_scanner] netif: found AP, initiating connection\n");
            int ret = nimble_netif_connect(addr, &_conn_params);
            if (ret < 0) {
                LOG_DEBUG("[ble_scanner] netif: unable to connect ret =%d\n", ret);
                if (ble_scanner_is_enabled()) {
                    LOG_DEBUG("[ble_scanner] netif: restart scanner\n");
                    nimble_scanner_start();
                }
            }
        }
    }
}
static ble_scan_listener_t _netif_listener = {
    .cb = _netif_cb
};

bool ble_scanner_netif_connected(void)
{
    return _state == STATE_CONNECTED;
}

void ble_scanner_netif_init(void)
{
    /* register our event callback */
    nimble_netif_eventcb(_on_netif_evt);
    ble_scanner_register(&_netif_listener);
    /* LOW_POWER config */
    ble_scanner_update(&ble_scan_params[1]);
    /* set initial parameters */
    ble_scanner_netif_update(&ble_scan_netif_params[0]);
    if (IS_ACTIVE(CONFIG_BLE_SCANNER_AUTO_START)) {
        ble_scanner_start(BLE_HS_FOREVER);
    }
}

void ble_scanner_netif_update(const ble_scan_netif_params_t *params)
{
    /* populate the connection parameters */
    memset(&_conn_params, 0, sizeof(_conn_params));
    _conn_params.scan_itvl_ms = params->scan_itvl_ms;
    _conn_params.scan_window_ms = params->scan_win_ms;
    _conn_params.conn_itvl_min_ms = params->conn_itvl_min_ms;
    _conn_params.conn_itvl_max_ms = params->conn_itvl_max_ms;
    _conn_params.conn_supervision_timeout_ms = params->conn_super_to_ms;
    _conn_params.conn_slave_latency = params->conn_latency_ms;
    _conn_params.timeout_ms = params->conn_timeout_ms;
    _conn_params.phy_mode = NIMBLE_PHY_1M;
    _conn_params.own_addr_type = nimble_riot_own_addr_type;

    /* we use the same values to updated existing connections */
    struct ble_gap_upd_params conn_update_params;

    conn_update_params.itvl_min = BLE_GAP_CONN_ITVL_MS(params->conn_itvl_min_ms);
    conn_update_params.itvl_max = BLE_GAP_CONN_ITVL_MS(params->conn_itvl_max_ms);
    conn_update_params.latency = params->conn_latency_ms;
    conn_update_params.supervision_timeout =
        BLE_GAP_SUPERVISION_TIMEOUT_MS(params->conn_super_to_ms);
    conn_update_params.min_ce_len = 0;
    conn_update_params.max_ce_len = 0;

    /* we also need to apply the new connection parameters to all BLE
     * connections where we are in the MASTER role */
    nimble_netif_conn_foreach(NIMBLE_NETIF_GAP_MASTER, _conn_update,
                              &conn_update_params);
}
