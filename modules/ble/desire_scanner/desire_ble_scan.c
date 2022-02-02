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
 * @author      Roudy Dagher <roudy.dagher@inria.fr>
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include "ztimer.h"

#include "desire_ble_scan.h"
#include "net/bluetil/ad.h"
#include "net/bluetil/addr.h"

#define ENABLE_DEBUG    0
#include "debug.h"

static detection_cb_t _detection_cb = NULL;

static void _desire_ble_stop_cb(void *arg)
{
    (void)arg;
    desire_ble_scan_stop();
}
static ztimer_t _timeout = { .callback = _desire_ble_stop_cb };

void _desire_cb(uint8_t adv_type, const ble_addr_t *addr,
                uint32_t ts, const nimble_scanner_info_t *info,
                const bluetil_ad_t *ad, void *arg)
{
    (void)arg;

    const uint16_t desire_uuid = DESIRE_SERVICE_UUID16;
    bluetil_ad_data_t field;             /*= {.data=desire_adv_payload.bytes,
                                              .len=DESIRE_ADV_PAYLOAD_SIZE}; */

    /* only interested in NONCONN adv */
    if (adv_type == BLE_HCI_ADV_TYPE_ADV_NONCONN_IND) {
        /* make sure the uid matches */
        if (bluetil_ad_find_and_cmp(ad, BLE_GAP_AD_UUID16_COMP,
                                    &desire_uuid,
                                    sizeof(uint16_t))) {
            /* make sure the data matches the expectation */
            if (BLUETIL_AD_OK ==
                bluetil_ad_find(ad, BLE_GAP_AD_SERVICE_DATA_UUID16,
                                &field)) {
                desire_ble_adv_payload_t *desire_adv_payload;
                DEBUG_PUTS("[desire_scanner]: found desire adv data");
                desire_adv_payload = (desire_ble_adv_payload_t *)field.data;
                if ((desire_adv_payload->data.service_uuid_16 ==
                     DESIRE_SERVICE_UUID16) &&
                    (_detection_cb != NULL)) {
                    _detection_cb(ts, addr, info->rssi, desire_adv_payload);
                }
            }
            else {
                DEBUG_PUTS("[desire_scanner]: malformed desire adv packet");
            }
        }
    }
}
static ble_scan_listener_t _desire_listener = {
    .cb = _desire_cb
};

void desire_ble_scan_init(detection_cb_t cb)
{
    desire_ble_set_detection_cb(cb);
}

void desire_ble_scan_start(const ble_scan_params_t *params, int32_t scan_duration_ms)
{
    ztimer_set(ZTIMER_MSEC, &_timeout, scan_duration_ms);

    ble_scanner_register(&_desire_listener);
    ble_scanner_update(params);
    ble_scanner_start(BLE_HS_FOREVER);
}

void desire_ble_scan_stop(void)
{
    ztimer_remove(ZTIMER_MSEC, &_timeout);
    ble_scanner_unregister(&_desire_listener);
    DEBUG_PUTS("[desire_scanner]: stop");
}

void desire_ble_set_detection_cb(detection_cb_t cb)
{
    assert(cb);
    _detection_cb = cb;
}
