/*
 * Copyright (C) 2022 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     ble_scanner
 *
 * @{
 * @file
 *
 */

#ifndef BLE_SCANNER_NETIF_PARAMS_H
#define BLE_SCANNER_NETIF_PARAMS_H

#include "ble_scanner.h"
#include "kernel_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Default parameters used for scan connections
 * @{
 */
/** @brief BLE netif default scan interval in ms */
#ifndef BLE_SCANNER_NETIF_SCAN_ITVL_MS
#define BLE_SCANNER_NETIF_SCAN_ITVL_MS   (1500U)          /* 2048ms */
#endif
/** @brief BLE netif default scan window in ms */
#ifndef BLE_SCANNER_NETIF_SCAN_WIN_MS
#define BLE_SCANNER_NETIF_SCAN_WIN_MS    (110U)           /*  256ms */
#endif
/** @brief BLE netif connection timeout */
#ifndef BLE_SCANNER_NETIF_TIMEOUT_MS
#define BLE_SCANNER_NETIF_TIMEOUT_MS     (3 * BLE_SCANNER_NETIF_SCAN_WIN_MS)
#endif
/** @brief BLE netif min. connection interval in ms */
#ifndef BLE_SCANNER_NETIF_ITVL_MIN_MS
#define BLE_SCANNER_NETIF_ITVL_MIN_MS    75U             /* 75ms */
#endif
/** @brief BLE netif max. connection interval in ms */
#ifndef BLE_SCANNER_NETIF_ITVL_MAX_MS
#define BLE_SCANNER_NETIF_ITVL_MAX_MS    75U             /* 75ms */
#endif
/** @brief BLE scanner latency */
#ifndef BLE_SCANNER_NETIF_LATENCY
#define BLE_SCANNER_NETIF_LATENCY        (0)
#endif
/** @brief BLE scanner ... */
#ifndef BLE_SCANNER_NETIF_SVTO_MS
#define BLE_SCANNER_NETIF_SVTO_MS        (2500U)         /* 2.5s */
#endif
/** @brief Default BLE scanner netif params */
#define BLE_SCANNER_NETIF_PARAMS                              \
    { .conn_timeout_ms = BLE_SCANNER_NETIF_TIMEOUT_MS,        \
      .conn_itvl_min_ms = BLE_SCANNER_NETIF_ITVL_MIN_MS,      \
      .conn_itvl_max_ms = BLE_SCANNER_NETIF_ITVL_MAX_MS,      \
      .conn_latency_ms = BLE_SCANNER_NETIF_LATENCY,           \
      .conn_super_to_ms = BLE_SCANNER_NETIF_SVTO_MS,          \
      .scan_win_ms = BLE_SCANNER_NETIF_SCAN_WIN_MS,           \
      .scan_itvl_ms = BLE_SCANNER_NETIF_SCAN_ITVL_MS,        \
    }
/** @} */

/**
 * @brief   ble_scan_netif_params configuration
 */
static const ble_scan_netif_params_t ble_scan_netif_params[] =
{
    BLE_SCANNER_NETIF_PARAMS
};

#ifdef __cplusplus
}
#endif

#endif /* BLE_SCANNER_NETIF_PARAMS_H */
/** @} */
