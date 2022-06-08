/*
 * Copyright (C) 2022 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_pepper_srv
 *
 * @brief       Serialization utilities
 *
 * @{
 *
 * @file
 *
 * @author      Anonymous
 */

#ifndef PEPPER_SERVER_STORAGE_H
#define PEPPER_SERVER_STORAGE_H

#include "epoch.h"
#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief epoch data file name */
#ifndef CONFIG_PEPPER_SRV_STORAGE_EPOCH_FILE
#define CONFIG_PEPPER_SRV_STORAGE_EPOCH_FILE            "epoch"
#endif
/** @brief uwb data file name */
#ifndef CONFIG_PEPPER_SRV_STORAGE_UWB_DATA_FILE
#define CONFIG_PEPPER_SRV_STORAGE_UWB_DATA_FILE         "uwb"
#endif
/** @brief ble data file name */
#ifndef CONFIG_PEPPER_SRV_STORAGE_BLE_DATA_FILE
#define CONFIG_PEPPER_SRV_STORAGE_BLE_DATA_FILE         "ble"
#endif

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_SERVER_STOAGE_H */
/** @} */
