/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_desire_ble_adv EBID advertisment manager
 * @ingroup     sys
 * @brief       Desire BLE EBID advertisement in Carousel mode.
 *
 *
 * @{
 *
 * @file
 *
 * @author      Roudy Dagher <roudy.dagher@inria.fr>
 */

#ifndef DESIRE_BLE_SCAN_H
#define DESIRE_BLE_SCAN_H

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "ble_scanner.h"
#include "host/ble_hs.h"
#include "desire/ble_pkt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Callback signature triggered by this module for each discovered
 *          advertising Desire packet
 *
 * @param[in] ts            Timestamp in ms since boot
 * @param[in] addr          BLE advertising address of the source node
 * @param[in] rssi          RSSI value for the received packet
 * @param[in] adv_payload   Decoded desire payload
 */
typedef void (*detection_cb_t)(uint32_t ts, const ble_addr_t *addr, int8_t rssi,
                               const desire_ble_adv_payload_t *adv_payload);

/**
 * @brief       Initialize the scanning module internal structure and scanning thread.
 *
 * @param[in]   cb            callback to register, may not be null
 */
void desire_ble_scan_init(detection_cb_t cb);

/**
 * @brief       Scans Desire packets and reports detection blocking call.
 *
 * Triggers a scan, and filters Desire packets, then reports decoded Desire payload.
 *
 * @param[in] params                new parameters to apply
 * @param[in] scan_duration_ms      The scan window duration in miliseconds
 *
 */
void desire_ble_scan_start(const ble_scan_params_t *params, int32_t scan_duration_ms);

/**
 * @brief       Stops any ongoing scan.
 *
 */
void desire_ble_scan_stop(void);

/**
 * @brief   Sets the callback called on each scanned desire advertisement packet
 *
 * @param[in]   cb            the callback may not be null
 */
void desire_ble_set_detection_cb(detection_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif /* DESIRE_BLE_SCAN_H */
/** @} */
