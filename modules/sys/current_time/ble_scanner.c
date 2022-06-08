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

#include "ble_scanner.h"
#include "ble_scanner_params.h"
#include "current_time.h"

#include "net/bluetil/ad.h"
#include "net/bluetil/addr.h"
#include "host/ble_hs.h"
#include "nimble/hci_common.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

void _current_time_cb(uint8_t adv_type, const ble_addr_t *addr,
                      uint32_t ts, const nimble_scanner_info_t *info,
                      const bluetil_ad_t *ad, void *arg)
{
    (void)addr;
    (void)info;
    (void)arg;

    const uint16_t cts_uuid = CURRENT_TIME_SERVICE_UUID16;
    bluetil_ad_data_t field;

    /* only interested in NONCONN adv */
    if (adv_type == BLE_HCI_ADV_TYPE_ADV_NONCONN_IND) {
        /* make sure the uid matches */
        if (bluetil_ad_find_and_cmp(ad, BLE_GAP_AD_UUID16_COMP,
                                    &cts_uuid,
                                    sizeof(uint16_t))) {
            /* make sure the data matches the expectation */
            if (BLUETIL_AD_OK ==
                bluetil_ad_find(ad, BLE_GAP_AD_SERVICE_DATA, &field)) {
                current_time_t *time = (current_time_t *)field.data;
                uint32_t elapsed = ztimer_now(ZTIMER_MSEC) - ts;
                current_time_update(time->epoch + elapsed);
            }
            else {
                LOG_DEBUG("[current_time]: malformed current time adv packet\n");
            }
        }
    }
}
static ble_scan_listener_t _current_time_listener = {
    .cb = _current_time_cb
};

void current_time_init_ble_scanner(void)
{
    ble_scanner_register(&_current_time_listener);
    ble_scanner_update(&ble_scan_params[2]);
    if (IS_ACTIVE(CONFIG_BLE_SCANNER_AUTO_START)) {
        ble_scanner_start(BLE_HS_FOREVER);
    }
}
