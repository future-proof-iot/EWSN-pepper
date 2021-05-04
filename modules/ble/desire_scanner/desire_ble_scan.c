#include "desire_ble_scan.h"

#include "nimble_scanner.h"
#include "xtimer.h"

#include "net/bluetil/ad.h"
#include "host/ble_hs.h"
#include "nimble/hci_common.h"

#include "ble_pkt_dbg.h"

detection_cb_t _detection_cb = NULL;

void _nimble_scanner_cb(uint8_t type,
                        const ble_addr_t *addr, int8_t rssi,
                        const uint8_t *adv, size_t adv_len)
{
    assert(addr);
    assert(adv_len <= BLE_ADV_PDU_LEN);

    uint32_t now = xtimer_now().ticks32;

    bluetil_ad_t ad = BLUETIL_AD_INIT((uint8_t*) adv, adv_len, adv_len);

    // dummy print ts: sender, rssi, adv data
    printf("t=%ld: rssi = %d, adv_type = %s, ", now, rssi, dbg_parse_ble_adv_type(type));
    dbg_print_ble_addr(addr);
    dbg_dump_buffer("\t adv_pkt = ", adv, adv_len, '\n');
    

    // filter on type: BLE_HCI_ADV_TYPE_ADV_NONCONN_IND
    if (type != BLE_HCI_ADV_TYPE_ADV_NONCONN_IND) {
        return;
    }

    // filter service uuid: BLE_GAP_AD_UUID16_COMP set to DESIRE_SERVICE_UUID16
    uint16_t desire_uuid = DESIRE_SERVICE_UUID16;

    if (!bluetil_ad_find_and_cmp(&ad, BLE_GAP_AD_UUID16_COMP, &desire_uuid,
                                 sizeof(uint16_t))) {
        printf("[Miss] DESIRE_SERVICE_UUID16 not matched in adv service uuid\n");
        return;
    }

    // filter desire service data field 
    desire_ble_adv_payload_t* desire_adv_payload;
    bluetil_ad_data_t field ;//= {.data=desire_adv_payload.bytes, .len=DESIRE_ADV_PAYLOAD_SIZE};

    if (BLUETIL_AD_OK == bluetil_ad_find(&ad, BLE_GAP_AD_SERVICE_DATA_UUID16, &field)) {
        printf("[Hit] Desire adv packet found, payload decoded\n");
        desire_adv_payload = (desire_ble_adv_payload_t*) field.data;
        // TODO check UUID of desire packet to DESIRE_SERVICE_UUID16
        // Callback
        if (_detection_cb != NULL) {
            _detection_cb(now, addr, rssi, desire_adv_payload);
        }
    } else {
        printf(
            "[Miss] DESIRE_SERVICE_UUID16 found in adv service uuid but missing or malformed service data field\n");
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
    printf("nimble_scanner_init ret =%d\n", ret);
    assert(ret == NIMBLE_SCANNER_OK);

}


void desire_ble_scan(uint32_t scan_duration_us,
                     detection_cb_t detection_cb)
{
    int ret;

    _detection_cb = detection_cb;

    //assert(nimble_scanner_status() == NIMBLE_SCANNER_STOPPED);

    // start scan
    ret = nimble_scanner_start();
    printf("nimble_scanner_start ret =%d\n", ret);
    assert(ret == NIMBLE_SCANNER_OK);

    // sleep
    printf("xtimer sleep for %ld usec\n", scan_duration_us);
    xtimer_usleep(scan_duration_us);
    printf("Done sleeping\n");

    // stop scan
    nimble_scanner_stop();
    _detection_cb = NULL;

    (void)ret;
}
