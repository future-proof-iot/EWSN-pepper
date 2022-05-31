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
/** @brief BLE low latency scan interval in ms */
#ifndef CONFIG_BLE_LOW_LATENCY_SCAN_ITVL_MS
#define CONFIG_BLE_LOW_LATENCY_SCAN_ITVL_MS        (4096U)          /* 4096ms */
#endif
/** @brief BLE low latency scan window in ms */
#ifndef CONFIG_BLE_LOW_LATENCY_SCAN_WIN_MS
#define CONFIG_BLE_LOW_LATENCY_SCAN_WIN_MS         (4096U)          /* 4096ms */
#endif
/** @brief BLE low latency scan parameters */
#define CONFIG_BLE_LOW_LATENCY_PARAMS                     \
    { .itvl_ms = CONFIG_BLE_LOW_LATENCY_SCAN_ITVL_MS,     \
      .win_ms = CONFIG_BLE_LOW_LATENCY_SCAN_WIN_MS,       \
    }
/** @} */

/**
 * @name    Low Power Scan Profile, 10%
 *
 * @note    Source: https://android.googlesource.com/platform/packages/apps/Bluetooth/+/master/src/com/android/bluetooth/gatt/ScanManager.java
 * @{
 */
/** @brief BLE low power scan interval in ms */
#ifndef CONFIG_BLE_LOW_POWER_SCAN_ITVL_MS
#define CONFIG_BLE_LOW_POWER_SCAN_ITVL_MS          (5120U)          /* 5120ms */
#endif
/** @brief BLE low power scan window in ms */
#ifndef CONFIG_BLE_LOW_POWER_SCAN_WIN_MS
#define CONFIG_BLE_LOW_POWER_SCAN_WIN_MS            (512U)          /*  512ms */
#endif
/** @brief BLE low power scan parameters */
#define CONFIG_BLE_LOW_POWER_PARAMS                       \
    { .itvl_ms = CONFIG_BLE_LOW_POWER_SCAN_ITVL_MS,       \
      .win_ms = CONFIG_BLE_LOW_POWER_SCAN_WIN_MS,         \
    }
/** @} */

/**
 * @name    Balanced San Profile, 25%
 *
 * @note    Source: https://android.googlesource.com/platform/packages/apps/Bluetooth/+/master/src/com/android/bluetooth/gatt/ScanManager.java
 * @{
 */
/** @brief BLE balanced scan interval in ms */
#ifndef CONFIG_BLE_BALANCED_SCAN_ITVL_MS
#define CONFIG_BLE_BALANCED_SCAN_ITVL_MS         (5120U)          /* 5120ms */
#endif
/** @brief BLE balanced scan window in ms */
#ifndef CONFIG_BLE_BALANCED_SCAN_WIN_MS
#define CONFIG_BLE_BALANCED_SCAN_WIN_MS          (1280U)          /* 1280ms */
#endif
/** @brief BLE balanced can parameters */
#define CONFIG_BLE_BALANCED_PARAMS                        \
    { .itvl_ms = CONFIG_BLE_BALANCED_SCAN_ITVL_MS,        \
      .win_ms = CONFIG_BLE_BALANCED_SCAN_WIN_MS,          \
    }
/** @} */

/**
 * @brief   default scan configurations
 */
static const ble_scan_params_t ble_scan_params[] =
{
    CONFIG_BLE_LOW_LATENCY_PARAMS,
    CONFIG_BLE_BALANCED_PARAMS,
    CONFIG_BLE_LOW_POWER_PARAMS
};

#ifdef __cplusplus
}
#endif

#endif /* BLE_SCANNER_PARAMS_H */
/** @} */
