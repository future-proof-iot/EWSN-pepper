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
 * @brief   The default EBID slice rotation period in secondS
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

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
/** @} */
