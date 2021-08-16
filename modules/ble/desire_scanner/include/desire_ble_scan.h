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

#include "host/ble_hs.h"
#include "desire_ble_pkt.h"
#include "time_ble_pkt.h"
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
#include "nimble_netif.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief   Set of configuration parameters needed to run autoconn
 */
typedef struct {
    /** scan interval applied while in scanning state [in ms] */
    uint32_t scan_itvl;
    /** scan window applied while in scanning state [in ms] */
    uint32_t scan_win;
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
    /** opening a new connection is aborted after this time [in ms] */
    uint32_t conn_timeout;
    /** connection interval used when opening a new connection, lower bound.
     *  [in ms] */
    uint32_t conn_itvl_min;
    /** connection interval, upper bound [in ms] */
    uint32_t conn_itvl_max;
    /** slave latency used for new connections [in ms] */
    uint16_t conn_latency;
    /** supervision timeout used for new connections [in ms] */
    uint32_t conn_super_to;
#endif
} desire_ble_scanner_params_t;

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
 * @param[in] params        new parameters to apply
 * @param[in] cb            callback to register, may not be null
 */
void desire_ble_scan_init(const desire_ble_scanner_params_t *params,
                          detection_cb_t cb);

/**
 * @brief       Scans Desire packets and reports detection blocking call.
 *
 * Triggers a scan, and filters Desire packets, then reports decoded Desire payload.
 *
 * @param[in]       scan_duration_ms    The scan window duration in miliseconds
 *
 */
void desire_ble_scan_start(int32_t scan_duration_ms);

/**
 * @brief   Updates the used parameters
 *
 * @param[in] params        new parameters to apply
 */
void desire_ble_scan_update(const desire_ble_scanner_params_t *params);

/**
 * @brief       Stops any ongoing scan.
 *
 */
void desire_ble_scan_stop(void);

/**
 * @brief   Sets a callback or each discovered advertising Current Time Service packet
 *
 * @param[in] callback   user callback with decode time structure
 */
void desire_ble_set_time_update_cb(time_update_cb_t cb);

/**
 * @brief   Sets the callback called on each scanned desire advertisement packet
 *
 * @param[in] cb            the callback may not be null
 */
void desire_ble_set_detection_cb(detection_cb_t cb);

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
/**
 * @brief   If the there is a ipv6 connection
 *
 * @return  true if connected, false otherwise
 */
bool desire_ble_is_connected(void);

/**
 * @brief   Register a callback that is called on netif events
 *
 * @param[in] cb            event callback to register
 */
void desire_ble_set_netif_cb(nimble_netif_eventcb_t cb);
#endif

#ifdef __cplusplus
}
#endif

#endif /* DESIRE_BLE_SCAN_H */
/** @} */
