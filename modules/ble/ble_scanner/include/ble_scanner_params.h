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

#ifndef BLE_SCANNER_PARAMS_H
#define BLE_SCANNER_PARAMS_H

#include "ble_scanner.h"
#include "kernel_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Low Latency Scan Profile, 100%
 *
 * @note    Source: https://android.googlesource.com/platform/packages/apps/Bluetooth/+/master/src/com/android/bluetooth/gatt/ScanManager.java
 * @{
 */
#ifndef BLE_SCANNER_LOW_LATENCY_SCAN_ITVL_MS
#define BLE_SCANNER_LOW_LATENCY_SCAN_ITVL_MS        (4096U)          /* 4096ms */
#endif
#ifndef BLE_SCANNER_LOW_LATENCY_SCAN_WIN_MS
#define BLE_SCANNER_LOW_LATENCY_SCAN_WIN_MS         (4096U)          /* 4096ms */
#endif

/**
 * @name    Low Power Scan Profile, 10%
 *
 * @note    Source: https://android.googlesource.com/platform/packages/apps/Bluetooth/+/master/src/com/android/bluetooth/gatt/ScanManager.java
 * @{
 */
#ifndef BLE_SCANNER_LOW_POWER_SCAN_ITVL_MS
#define BLE_SCANNER_LOW_POWER_SCAN_ITVL_MS          (5120U)          /* 5120ms */
#endif
#ifndef BLE_SCANNER_LOW_POWER_SCAN_WIN_MS
#define BLE_SCANNER_LOW_POWER_SCAN_WIN_MS            (512U)          /*  512ms */
#endif


/**
 * @name    Balanced San Profile, 25%
 *
 * @note    Source: https://android.googlesource.com/platform/packages/apps/Bluetooth/+/master/src/com/android/bluetooth/gatt/ScanManager.java
 * @{
 */
#ifndef BLE_SCANNER_BALANCED_SCAN_ITVL_MS
#define BLE_SCANNER_BALANCED_SCAN_ITVL_MS         (4096U)          /* 4096ms */
#endif
#ifndef BLE_SCANNER_BALANCED_SCAN_WIN_MS
#define BLE_SCANNER_BALANCED_SCAN_WIN_MS          (1024U)          /* 1024ms */
#endif

/**
 * @name    Default parameters used for scan connections
 * @{
 */
#ifndef BLE_SCANNER_CONN_TIMEOUT_MS
#define BLE_SCANNER_CONN_TIMEOUT_MS     (3 * BLE_SCANNER_LOW_POWER_SCAN_WIN_MS)
#endif
#ifndef BLE_SCANNER_CONN_ITVL_MIN_MS
#define BLE_SCANNER_CONN_ITVL_MIN_MS    75U             /* 75ms */
#endif
#ifndef BLE_SCANNER_CONN_ITVL_MAX_MS
#define BLE_SCANNER_CONN_ITVL_MAX_MS    75U             /* 75ms */
#endif
#ifndef BLE_SCANNER_CONN_LATENCY
#define BLE_SCANNER_CONN_LATENCY        (0)
#endif
#ifndef BLE_SCANNER_CONN_SVTO_MS
#define BLE_SCANNER_CONN_SVTO_MS        (2500U)         /* 2.5s */
#endif

#define BLE_SCANNER_CONN_PARAMS                           \
    { .conn_timeout = BLE_SCANNER_CONN_TIMEOUT_MS,        \
      .conn_itvl_min = BLE_SCANNER_CONN_ITVL_MIN_MS,      \
      .conn_itvl_max = BLE_SCANNER_CONN_ITVL_MAX_MS,      \
      .conn_latency = BLE_SCANNER_CONN_LATENCY,           \
      .conn_super_to = BLE_SCANNER_CONN_SVTO_MS,          \
    }

#define BLE_SCANNER_LOW_LATENCY_PARAMS                    \
    { .scan_itvl = BLE_SCANNER_LOW_LATENCY_SCAN_ITVL_MS,  \
      .scan_win = BLE_SCANNER_LOW_LATENCY_SCAN_WIN_MS,    \
      .conn_params = BLE_SCANNER_CONN_PARAMS,             \
    }

#define BLE_SCANNER_BALANCED_PARAMS                       \
    { .scan_itvl = BLE_SCANNER_BALANCED_SCAN_ITVL_MS,     \
      .scan_win = BLE_SCANNER_BALANCED_SCAN_WIN_MS,       \
      .conn_params = BLE_SCANNER_CONN_PARAMS,             \
    }

#define BLE_SCANNER_LOW_POWER_PARAMS                     \
    { .scan_itvl = BLE_SCANNER_LOW_POWER_SCAN_ITVL_MS,   \
      .scan_win = BLE_SCANNER_LOW_POWER_SCAN_WIN_MS,     \
      .conn_params = BLE_SCANNER_CONN_PARAMS,            \
    }

/**
 * @brief   nimble_netif_autoconn configuration
 */
static const ble_scan_params_t ble_scan_params[] =
{
    BLE_SCANNER_LOW_LATENCY_PARAMS,
    BLE_SCANNER_BALANCED_PARAMS,
    BLE_SCANNER_LOW_POWER_PARAMS
};

#ifdef __cplusplus
}
#endif

#endif /* BLE_SCANNER_PARAMS_H */
/** @} */
