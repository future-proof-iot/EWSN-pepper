/*
 * Copyright (C) 2019 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     pkg_nimble_autoconn
 *
 * @{
 * @file
 * @brief       Default configuration for the nimble_autoconn module
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef DESIRE_SCANNER_PARAMS_H
#define DESIRE_SCANNER_PARAMS_H

#include "ble_scanner_params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   nimble_netif_autoconn configuration
 */
static const ble_scan_params_t desire_ble_scan_params = BLE_SCANNER_BALANCED_PARAMS;

#ifdef __cplusplus
}
#endif

#endif /* DESIRE_SCANNER_PARAMS_H */
/** @} */
