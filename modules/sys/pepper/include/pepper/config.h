/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_pepper PrEcise Privacy-PresERving Proximity Tracing (PEPPER)
 * @ingroup     sys
 * @brief       PrEcise Privacy-PresERving Proximity Tracing module
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   The default EBID slice rotation period in seconds
 *
 */
#ifndef CONFIG_ADV_PER_SLICE
#define CONFIG_ADV_PER_SLICE            (20LU)
#endif

/**
 * @brief   The default EBID slice rotation period in seconds
 *
 */
#ifndef CONFIG_EPOCH_DURATION_SEC
#define CONFIG_EPOCH_DURATION_SEC        (15LU * 60)
#endif

/**
 * @brief   The default EBID slice rotation period in seconds
 *
 */
#ifndef CONFIG_EBID_ROTATION_T_S
#define CONFIG_EBID_ROTATION_T_S        (CONFIG_EPOCH_DURATION_SEC)
#endif

/**
 * @brief   Values above this values will be clipped before being averaged
 */
#define RSSI_CLIPPING_THRESH    (0)

/**
 * @name    Default ble scan parameters
 *
 * @note    Source: https://android.googlesource.com/platform/packages/apps/Bluetooth/+/master/src/com/android/bluetooth/gatt/ScanManager.java
 * @{
 */
#ifndef CONFIG_BLE_SCAN_ITVL_MS
#define CONFIG_BLE_SCAN_ITVL_MS        (5120U)  /* 5120ms */
#endif
#ifndef CONFIG_BLE_SCAN_WIN_MS
#define CONFIG_BLE_SCAN_WIN_MS         (1280U)  /* 1280ms */
#endif

#ifndef CONFIG_BLE_SCAN_PARAMS
#define CONFIG_BLE_SCAN_PARAMS                      \
    { .scan_itvl_ms = CONFIG_BLE_SCAN_ITVL_MS,      \
      .scan_win_ms = CONFIG_BLE_SCAN_WIN_MS,        \
    }
#endif

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
/** @} */
