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

#include "desire_ble_scan.h"
#include "kernel_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Default parameters used for the desire_ble_scan module
 * @{
 */
/* TODO: curent values means its allways scanning, but currently this has a high impact
on ranging so keep it as is for now */
#ifndef DESIRE_SCANNER_SCAN_ITVL_MS
#define DESIRE_SCANNER_SCAN_ITVL_MS        (220U)          /* 220ms */
#endif
#ifndef DESIRE_SCANNER_SCAN_WIN_MS
#define DESIRE_SCANNER_SCAN_WIN_MS         (220U)          /* 110ms */
#endif
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
#ifndef DESIRE_SCANNER_CONN_TIMEOUT_MS
#define DESIRE_SCANNER_CONN_TIMEOUT_MS     (3 * DESIRE_SCANNER_SCAN_WIN_MS)
#endif
#ifndef DESIRE_SCANNER_CONN_ITVL_MIN_MS
#define DESIRE_SCANNER_CONN_ITVL_MIN_MS    75U             /* 75ms */
#endif
#ifndef DESIRE_SCANNER_CONN_ITVL_MAX_MS
#define DESIRE_SCANNER_CONN_ITVL_MAX_MS    75U             /* 75ms */
#endif
#ifndef DESIRE_SCANNER_CONN_LATENCY
#define DESIRE_SCANNER_CONN_LATENCY        (0)
#endif
#ifndef DESIRE_SCANNER_CONN_SVTO_MS
#define DESIRE_SCANNER_CONN_SVTO_MS        (2500U)         /* 2.5s */
#endif
#endif
#ifndef DESIRE_SCANNER_PARAMS
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
#define DESIRE_SCANNER_PARAMS                           \
    { .scan_itvl     = DESIRE_SCANNER_SCAN_ITVL_MS,     \
      .conn_timeout  = DESIRE_SCANNER_CONN_TIMEOUT_MS,  \
      .scan_win      = DESIRE_SCANNER_SCAN_WIN_MS,      \
      .conn_itvl_min = DESIRE_SCANNER_CONN_ITVL_MIN_MS, \
      .conn_itvl_max = DESIRE_SCANNER_CONN_ITVL_MAX_MS, \
      .conn_latency  = DESIRE_SCANNER_CONN_LATENCY,     \
      .conn_super_to = DESIRE_SCANNER_CONN_SVTO_MS, }
#else
#define DESIRE_SCANNER_PARAMS                           \
    { .scan_itvl     = DESIRE_SCANNER_SCAN_ITVL_MS,     \
      .scan_win      = DESIRE_SCANNER_SCAN_WIN_MS, }
#endif
#endif
/**@}*/

/**
 * @brief   nimble_netif_autoconn configuration
 */
static const desire_ble_scanner_params_t desire_ble_scanner_params =
    DESIRE_SCANNER_PARAMS;

#ifdef __cplusplus
}
#endif

#endif /* DESIRE_SCANNER_PARAMS_H */
/** @} */
