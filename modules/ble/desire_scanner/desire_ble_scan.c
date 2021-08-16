#include "desire_ble_scan.h"
#include "nimble_scanner.h"
#include "ztimer.h"

#include "net/bluetil/ad.h"
#include "net/bluetil/addr.h"
#include "host/ble_hs.h"
#include "nimble/hci_common.h"

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
#include "nimble_netif.h"
#include "nimble_netif_conn.h"
#endif

#include "ble_pkt_dbg.h"

#define ENABLE_DEBUG    0
#include "debug.h"

static detection_cb_t _detection_cb = NULL;
#if IS_USED(MODULE_DESIRE_SCANNER_TIME_UPDATE)
static time_update_cb_t _time_update_cb = NULL;
#endif
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
static bool _scan_restart = false;
static uint32_t _scan_end_ms = 0;
static nimble_netif_eventcb_t _netif_cb = NULL;
static struct ble_gap_conn_params _conn_params;
static uint32_t _conn_timeout;
#endif

enum {
    STATE_IDLE,
    STATE_CONNECTED,
    STATE_CONNECTING,
    STATE_CLOSED,
};
static volatile uint8_t _state = STATE_IDLE;

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
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

static void _handle_netif_data(const ble_addr_t *addr, bluetil_ad_t *ad)
{
    /* for connection checking we need the address in network byte order */
    uint8_t addrn[BLE_ADDR_LEN];

    bluetil_addr_swapped_cp(addr->val, addrn);

    if (_filter_uuid(ad) && !nimble_netif_conn_connected(addrn) &&
        !desire_ble_is_connected()) {
        nimble_scanner_stop();
        DEBUG_PUTS("[desire_scanner]: found AP, initiating connection");
        int ret = nimble_netif_connect(addr, &_conn_params, _conn_timeout);
        if (ret < 0) {
            DEBUG("[desire_scanner]: unable to connect ret =%d\n", ret);
            uint32_t now = ztimer_now(ZTIMER_MSEC);
            if ((int32_t)_scan_end_ms - now > 0) {
                int32_t scan_rem = _scan_end_ms - now;
                nimble_scanner_set_scan_duration(scan_rem);
                nimble_scanner_start();
            }
        }
        /* scanning needs to be resumed once connection is handled */
        _scan_restart = true;
    }
}
#endif

#if IS_USED(MODULE_DESIRE_SCANNER_TIME_UPDATE)
static void _handle_time_service_data(bluetil_ad_t *ad)
{
    bluetil_ad_data_t field; /*= {.data=desire_adv_payload.bytes,
                                  .len=DESIRE_ADV_PAYLOAD_SIZE}; */

    if (BLUETIL_AD_OK ==
        bluetil_ad_find(ad, BLE_GAP_AD_SERVICE_DATA_UUID16, &field)) {
        current_time_ble_adv_payload_t *cts_adv_payload =
            (current_time_ble_adv_payload_t *)field.data;
        DEBUG_PUTS("[desire_scanner]: current time adv packet found");
        if ((cts_adv_payload->data.service_uuid_16 ==
             CURRENT_TIME_SERVICE_UUID16) && (_time_update_cb != NULL)) {
            DEBUG_PUTS("\t Calling user callback");
            _time_update_cb(cts_adv_payload);
        }
        else {
            DEBUG("\t Failure service uuid = %04X and callback = %d\n",
                  cts_adv_payload->data.service_uuid_16,
                  _time_update_cb != NULL);
        }
    }
    else {
        DEBUG_PUTS("[desire_scanner]: malformed current time adv packet");
    }
}
#endif

static void _on_scan_evt(uint8_t type, const ble_addr_t *addr, int8_t rssi,
                         const uint8_t *adv, size_t adv_len)
{
    assert(addr);
    assert(adv_len <= BLE_ADV_PDU_LEN);

    uint32_t now = ztimer_now(ZTIMER_MSEC);
    bluetil_ad_t ad = BLUETIL_AD_INIT((uint8_t *)adv, adv_len, adv_len);

    /* dummy print ts: sender, rssi, adv data */
    DEBUG("t=%ld: rssi = %d, adv_type = %s, ", now, rssi, dbg_parse_ble_adv_type(
              type));
    if (IS_ACTIVE(ENABLE_DEBUG)) {
        dbg_print_ble_addr(addr);
        dbg_dump_buffer("\t adv_pkt = ", adv, adv_len, '\n');
    }

    const uint16_t desire_uuid = DESIRE_SERVICE_UUID16;
#if IS_USED(MODULE_DESIRE_SCANNER_TIME_UPDATE)
    const uint16_t cts_uuid = CURRENT_TIME_SERVICE_UUID16;
#endif

    switch (type) {
        case BLE_HCI_ADV_TYPE_ADV_NONCONN_IND:
            if (bluetil_ad_find_and_cmp(&ad, BLE_GAP_AD_UUID16_COMP,
                                        &desire_uuid,
                                        sizeof(uint16_t))) {
                bluetil_ad_data_t field; /*= {.data=desire_adv_payload.bytes,
                                              .len=DESIRE_ADV_PAYLOAD_SIZE}; */
                if (BLUETIL_AD_OK ==
                    bluetil_ad_find(&ad, BLE_GAP_AD_SERVICE_DATA_UUID16,
                                    &field)) {
                    desire_ble_adv_payload_t *desire_adv_payload;
                    DEBUG_PUTS("[desire_scanner]: found desire adv data");
                    desire_adv_payload = (desire_ble_adv_payload_t *)field.data;
                    if ((desire_adv_payload->data.service_uuid_16 ==
                         DESIRE_SERVICE_UUID16) &&
                        (_detection_cb != NULL)) {
                        _detection_cb(now, addr, rssi, desire_adv_payload);
                    }
                }
                else {
                    DEBUG_PUTS("[desire_scanner]: malformed desire adv packet");
                }
            }
#if IS_USED(MODULE_DESIRE_SCANNER_TIME_UPDATE)
            else if (bluetil_ad_find_and_cmp(&ad, BLE_GAP_AD_UUID16_COMP,
                                             &cts_uuid,
                                             sizeof(uint16_t))) {
                _handle_time_service_data(&ad);
            }
#endif
            else {
                DEBUG_PUTS("[desire_scanner]: unsupported nonconn adv packet");
            }
            break;
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
        case BLE_HCI_ADV_TYPE_ADV_IND:
            _handle_netif_data(addr, &ad);
            break;
#endif
        default:
            DEBUG_PUTS("[desire_scanner]: unsupported adv packet");
            return;
    }
}

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
static void _evt_dbg(const char *msg, int handle, const uint8_t *addr)
{
    if (IS_ACTIVE(ENABLE_DEBUG)) {
        printf("[desire_scanner] %s (%i|", msg, handle);
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

    if (_scan_restart) {
        _scan_restart = false;
        uint32_t now = ztimer_now(ZTIMER_MSEC);
        /* restart only if there is some scan_time remaining */
        if ((int32_t)_scan_end_ms - now > 0) {
            int32_t scan_rem = _scan_end_ms - now;
            nimble_scanner_set_scan_duration(scan_rem);
            nimble_scanner_start();
        }
    }

    /* pass events to high-level user if someone registered for them */
    if (_netif_cb) {
        _netif_cb(handle, event, addr);
    }
}
#endif

void desire_ble_scan_init(const desire_ble_scanner_params_t *params,
                          detection_cb_t cb)
{
#if IS_USED(MODULE_DESIRE_SCANNER_TIME_UPDATE)
    _time_update_cb = NULL;
#endif
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
    _netif_cb = NULL;
    /* register our event callback */
    nimble_netif_eventcb(_on_netif_evt);
#endif
    desire_ble_set_detection_cb(cb);
    /* set the given parameters */
    return desire_ble_scan_update(params);
}

void desire_ble_scan_start(int32_t scan_duration_ms)
{
    /* stop previous ongoing scan if any */
    nimble_scanner_stop();
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
    nimble_netif_accept_stop();
#endif
    /* set scan duration */
    nimble_scanner_set_scan_duration(scan_duration_ms);
    /* start scanning */
    int ret = nimble_scanner_start();
    /* save time where scanning is supposed to end in case it needs
       to be restarted after a connection event */
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
    _scan_end_ms = ztimer_now(ZTIMER_MSEC) + scan_duration_ms;
#endif
    DEBUG("[desire_scanner]: start ret =%d\n", ret);
    assert(ret == NIMBLE_SCANNER_OK);
    (void)ret;
}

void desire_ble_scan_stop(void)
{
    nimble_scanner_stop();
    DEBUG_PUTS("[desire_scanner]: stop");
}

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
static int _conn_update(nimble_netif_conn_t *conn, int handle, void *arg)
{
    (void)conn;
    nimble_netif_update(handle, (const struct ble_gap_upd_params *)arg);
    return 0;
}
#endif

void desire_ble_scan_update(const desire_ble_scanner_params_t *params)
{
    /* calculate the used scan parameters */
    struct ble_gap_disc_params scan_params;

    scan_params.itvl = BLE_GAP_SCAN_ITVL_MS(params->scan_itvl);
    scan_params.window = BLE_GAP_SCAN_WIN_MS(params->scan_win);
    scan_params.filter_policy = 0;
    scan_params.limited = 0;
    scan_params.passive = 0;
    scan_params.filter_duplicates = 0;

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
    /* populate the connection parameters */
    _conn_params.scan_itvl = BLE_GAP_SCAN_ITVL_MS(params->scan_win);
    _conn_params.scan_window = _conn_params.scan_itvl;
    _conn_params.itvl_min = BLE_GAP_CONN_ITVL_MS(params->conn_itvl_min);
    _conn_params.itvl_max = BLE_GAP_CONN_ITVL_MS(params->conn_itvl_max);
    _conn_params.latency = 0;
    _conn_params.supervision_timeout = BLE_GAP_SUPERVISION_TIMEOUT_MS(
        params->conn_super_to);
    _conn_params.min_ce_len = 0;
    _conn_params.max_ce_len = 0;
    _conn_timeout = params->conn_timeout;

    /* we use the same values to updated existing connections */
    struct ble_gap_upd_params conn_update_params;
    conn_update_params.itvl_min = _conn_params.itvl_min;
    conn_update_params.itvl_max = _conn_params.itvl_max;
    conn_update_params.latency = _conn_params.latency;
    conn_update_params.supervision_timeout = _conn_params.supervision_timeout;
    conn_update_params.min_ce_len = 0;
    conn_update_params.max_ce_len = 0;
#endif

    int ret = nimble_scanner_init(&scan_params, _on_scan_evt);
    DEBUG("[desire_scanner]: init ret =%d\n", ret);
    assert(ret == NIMBLE_SCANNER_OK);
    (void)ret;

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
    /* we also need to apply the new connection parameters to all BLE
     * connections where we are in the MASTER role */
    nimble_netif_conn_foreach(NIMBLE_NETIF_GAP_MASTER, _conn_update,
                              &conn_update_params);
#endif
}

void desire_ble_set_detection_cb(detection_cb_t cb)
{
    assert(cb);
    _detection_cb = cb;
}

#if IS_USED(MODULE_DESIRE_SCANNER_TIME_UPDATE)
void desire_ble_set_time_update_cb(time_update_cb_t cb)
{
    _time_update_cb = cb;
}
#endif

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
void desire_ble_set_netif_cb(nimble_netif_eventcb_t cb)
{
    _netif_cb = cb;
}

bool desire_ble_is_connected(void)
{
    return _state == STATE_CONNECTED;
}
#endif
