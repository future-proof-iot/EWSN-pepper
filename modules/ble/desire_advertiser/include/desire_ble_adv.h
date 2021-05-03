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

#ifndef DESIRE_BLE_ADV_H
#define DESIRE_BLE_ADV_H

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "timex.h"

#include "desire_ble_pkt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DESIRE_DEFAULT_SLICE_ROTATION_PERIOD_SEC 20
#define DESIRE_DEFAULT_EBID_ROTATION_PERIOD_SEC (15 * SEC_PER_MIN)

#ifndef DESIRE_STATIC_EBID
    #define DESIRE_STATIC_EBID 0
#endif


/**
 * @brief       Initialize the advertising module internal structure and advertising thread.
 *
 */
void desire_ble_adv_init(void);

/**
 * @brief       Advertises an EBID following Desire Carousel scheme.
 *
 * The EBID is split in 3 slices with an extra XOR slice. Each slice is advertised every second for a duration slice_adv_time_sec.
 * The EBID is regenerated every ebid_adv_time_sec.
 *
 * @param[in]       slice_adv_time_sec      The rotation period (default to use in @ref DESIRE_DEFAULT_SLICE_ROTATION_PERIOD_SEC)
 * @param[in]       ebid_adv_time_sec       Interval in seconds for renewing the EBID (default to use in @ref DESIRE_DEFAULT_EBID_ROTATION_PERIOD_SEC)
 *
 */
void desire_ble_adv_start(uint16_t slice_adv_time_sec,
                          uint16_t ebid_adv_time_sec);

/**
 * @brief       Stops the advertisement loop.
 *
 */
void desire_ble_adv_stop(void);


#ifdef __cplusplus
}
#endif

#endif /* DESIRE_BLE_ADV_H */
/** @} */
