/*
 * Copyright (C) 2022 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    ble_scanner NimBLE Scanner Multiplexing Module
 * @ingroup     ble
 * @brief       A module to multiplex BLE request allowing different users
 *              to register interests
 *
 * @{
 *
 * @file
 *
 * @author      Anonymous
 * @author      Anonymous
 */

#ifndef BLE_SCANNER_H
#define BLE_SCANNER_H

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "net/bluetil/ad.h"
#include "net/bluetil/addr.h"
#include "nimble_scanner.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Start the BLE scanner on init
 */
#define CONFIG_BLE_SCANNER_AUTO_START       1

/**
 * @brief   Set of scan connection parameters
 */
typedef struct {
    /** scan interval applied while in scanning state [in ms] */
    uint32_t scan_itvl_ms;
    /** scan window applied while in scanning state [in ms] */
    uint32_t scan_win_ms;
    /** opening a new connection is aborted after this time [in ms] */
    uint32_t conn_timeout_ms;
    /** connection interval used when opening a new connection, lower bound.
     *  [in ms] */
    uint32_t conn_itvl_min_ms;
    /** connection interval, upper bound [in ms] */
    uint32_t conn_itvl_max_ms;
    /** slave latency used for new connections [in ms] */
    uint16_t conn_latency_ms;
    /** supervision timeout used for new connections [in ms] */
    uint32_t conn_super_to_ms;
} ble_scan_netif_params_t;

/**
 * @brief   Set of configuration parameters needed to run autoconn
 */
typedef struct {
    /** scan interval applied while in scanning state [in ms] */
    uint32_t itvl_ms;
    /** scan window applied while in scanning state [in ms] */
    uint32_t win_ms;
} ble_scan_params_t;

/**
 * @brief   BLE scan listener callback
 *
 * @param[in] adv_type      The advertisement type
 * @param[in] addr          BLE advertising address of the source node
 * @param[in] ts            Timestamp in ms
 * @param[in] info          Advertisement packet information
 * @param[in] ad            Advertisement data
 * @param[in] arg           Optional argument
 */
typedef void (*ble_scan_cb_t)(uint8_t adv_type, const ble_addr_t *addr,
                              uint32_t ts, const nimble_scanner_info_t *info,
                              const bluetil_ad_t *ad, void *arg);

/**
 * @brief   BLE scan listeners
 */
typedef struct ble_scan_listener {
    clist_node_t list_node;         /**< the list node item */
    ble_scan_cb_t cb;               /**< the callback */
    void *arg;                      /**< optional arg */
} ble_scan_listener_t;

/**
 * @brief   Register a listener for scan events
 *
 * @note    If the ble_scanner_was started this will re-start the nimblescanner
 *
 * @param[in]   listener    the listener
 */
void ble_scanner_register(ble_scan_listener_t *listener);

/**
 * @brief   Unregisters a listener for scan events
 *
 * @note    If the ble_scanner_was started and there are nor more registered
 *          listeners then this will stop the NimBLE scanner
 *
 * @param[in]   listener    the listener
 */
void ble_scanner_unregister(ble_scan_listener_t *listener);

/**
 * @brief   Initialize the scanning module internal structure
 *
 * @param[in]   params        new parameters to apply
 */
void ble_scanner_init(const ble_scan_params_t *params);

/**
 * @brief   Enable scanning for a duration.
 *
 * @note    If there is not registered listener the NimBLE scanner will not be
 *          started
 *
 * @param[in]   scan_duration_ms    The scan window duration in miliseconds
 *
 */
void ble_scanner_start(int32_t scan_duration_ms);

/**
 * @brief   Stops any ongoing scan.
 */
void ble_scanner_stop(void);

/**
 * @brief   Updates the used parameters
 *
 * @param[in] params        new parameters to apply
 */
void ble_scanner_update(const ble_scan_params_t *params);

/**
 * @brief   Returns if there is a connection
 *
 * @return  true if connected, false otherwise
 */
bool ble_scanner_is_enabled(void);

/**
 * @brief   Updates the used parameters
 *
 * @param[in] params        new parameters to apply
 */
void ble_scanner_netif_update(const ble_scan_netif_params_t *params);

/**
 * @brief   Initialize the netif scanning module
 */
void ble_scanner_netif_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_SCANNER_H */
/** @} */
