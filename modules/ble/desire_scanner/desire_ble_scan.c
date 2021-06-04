#include "desire_ble_scan.h"
#include "nimble_scanner.h"
#include "ztimer.h"

#include "net/bluetil/ad.h"
#include "host/ble_hs.h"
#include "nimble/hci_common.h"

#include "ble_pkt_dbg.h"

#define ENABLE_DEBUG    0
#include "debug.h"

static detection_cb_t _detection_cb = NULL;
static time_update_cb_t _time_update_cb = NULL;

static void _nimble_scanner_cb(uint8_t type, const ble_addr_t *addr,
                               int8_t rssi, const uint8_t *adv,
                               size_t adv_len)
{
    assert(addr);
    assert(adv_len <= BLE_ADV_PDU_LEN);

    uint32_t now = ztimer_now(ZTIMER_MSEC);

    bluetil_ad_t ad = BLUETIL_AD_INIT((uint8_t*) adv, adv_len, adv_len);

    // dummy print ts: sender, rssi, adv data
    DEBUG("t=%ld: rssi = %d, adv_type = %s, ", now, rssi, dbg_parse_ble_adv_type(type));
    if (IS_ACTIVE(ENABLE_DEBUG)) {
        dbg_print_ble_addr(addr);
        dbg_dump_buffer("\t adv_pkt = ", adv, adv_len, '\n');
    }


    // filter on type: BLE_HCI_ADV_TYPE_ADV_NONCONN_IND
    if (type != BLE_HCI_ADV_TYPE_ADV_NONCONN_IND) {
        return;
    }

    // filter service uuid: BLE_GAP_AD_UUID16_COMP set to DESIRE_SERVICE_UUID16
    uint16_t desire_uuid = DESIRE_SERVICE_UUID16;
    uint16_t cts_uuid =  CURRENT_TIME_SERVICE_UUID16;

    if (bluetil_ad_find_and_cmp(&ad, BLE_GAP_AD_UUID16_COMP, &desire_uuid,
                                sizeof(uint16_t))) {
        // filter desire service data field
        bluetil_ad_data_t field;     //= {.data=desire_adv_payload.bytes, .len=DESIRE_ADV_PAYLOAD_SIZE};

        if (BLUETIL_AD_OK == bluetil_ad_find(&ad, BLE_GAP_AD_SERVICE_DATA_UUID16, &field)) {
            desire_ble_adv_payload_t *desire_adv_payload;
            DEBUG("[Hit] Desire adv packet found, payload decoded\n");
            desire_adv_payload = (desire_ble_adv_payload_t *)field.data;
            // Callback if UUID of desire packet matches to DESIRE_SERVICE_UUID16
            if ((desire_adv_payload->data.service_uuid_16 == DESIRE_SERVICE_UUID16) &&
                (_detection_cb != NULL)) {
                _detection_cb(now, addr, rssi, desire_adv_payload);
            }
        }
        else {
            DEBUG(
                "[Miss] DESIRE_SERVICE_UUID16 found in adv service uuid but missing or malformed in service data field\n");
        }
    }
    else if (bluetil_ad_find_and_cmp(&ad, BLE_GAP_AD_UUID16_COMP, &cts_uuid,
                                     sizeof(uint16_t))) {
        // filter desire service data field
        bluetil_ad_data_t field;     //= {.data=desire_adv_payload.bytes, .len=DESIRE_ADV_PAYLOAD_SIZE};

        if (BLUETIL_AD_OK == bluetil_ad_find(&ad, BLE_GAP_AD_SERVICE_DATA_UUID16, &field)) {
            current_time_ble_adv_payload_t *cts_adv_payload;
            DEBUG("[Hit] Current Time adv packet found, payload decoded\n");
            cts_adv_payload = (current_time_ble_adv_payload_t *)field.data;
            // Callback if UUID of desire packet matches to CURRENT_TIME_SERVICE_UUID16
            if ((cts_adv_payload->data.service_uuid_16 == CURRENT_TIME_SERVICE_UUID16) &&
                (_time_update_cb != NULL)) {
                DEBUG("\t Calling user callback\n");
                _time_update_cb(cts_adv_payload);
            }
            else {
                DEBUG("\t Failure service uuid = %04X and callback = %d\n", cts_adv_payload->data.service_uuid_16, _time_update_cb!=NULL);
            }
        }
        else {
            DEBUG(
                "[Miss] CURRENT_TIME_SERVICE_UUID16 found in adv service uuid but missing or malformed in service data field\n");
        }
    }
    else {
        DEBUG("[Miss] unsupported advertising packet");
    }
}

void desire_ble_scan_init(void)
{
    //FIXME investigate window spec: .itvl, .window
    struct ble_gap_disc_params scan_params = {
        .itvl = BLE_GAP_LIM_DISC_SCAN_INT,
        .window = BLE_GAP_LIM_DISC_SCAN_WINDOW,
        .filter_policy = 0,                         /* don't use */
        .limited = 0,                               /* no limited discovery */
        .passive = 0,                               /* no passive scanning */
        .filter_duplicates = 0,                     /* no duplicate filtering */
    };
    int ret;

    ret = nimble_scanner_init(&scan_params, _nimble_scanner_cb);
    DEBUG("nimble_scanner_init ret =%d\n", ret);
    assert(ret == NIMBLE_SCANNER_OK);
}

void desire_ble_scan(uint32_t scan_duration_ms,
                     detection_cb_t detection_cb)
{
    // stop prevous ongoing scan if any
    nimble_scanner_stop();

    _detection_cb = detection_cb;
    nimble_scanner_set_scan_duration(scan_duration_ms);

    // trigger new scan
    int ret = nimble_scanner_start();
    DEBUG("nimble_scanner_start ret =%d\n", ret);
    assert(ret == NIMBLE_SCANNER_OK);
    (void) ret;
}

void desire_ble_scan_stop(void)
{
    nimble_scanner_stop();
}

void desire_ble_set_time_update_cb(time_update_cb_t usr_callback) {
    _time_update_cb = usr_callback;
}
