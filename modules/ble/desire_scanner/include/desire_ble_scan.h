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

#include "timex.h"

#include "host/ble_hs.h"
#include "desire_ble_pkt.h"

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
typedef void (*detection_cb_t)(uint32_t ts,
                               const ble_addr_t *addr, int8_t rssi,
                               const desire_ble_adv_payload_t *adv_payload);


/**
 * @brief       Initialize the scanning module internal structure and scanning thread.
 *
 */
void desire_ble_scan_init(void);

/**
 * @brief       Scans Desire packets and reports detection blocking call.
 *
 * Triggers a scan, and filters Desire packets, then reports decoded Desire payload.
 *
 * @param[in]       scan_duration_us    The scan window duration in micorseconds
 * @param[in]       detection_cb        Callback for each detected packet (offload asap).
 *
 */
void desire_ble_scan(uint32_t scan_duration_us,
                     detection_cb_t detection_cb);



#ifdef __cplusplus
}
#endif

#endif /* DESIRE_BLE_SCAN_H */
/** @} */
