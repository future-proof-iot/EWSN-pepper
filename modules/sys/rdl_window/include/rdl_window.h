/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_rdl_window Request Windowed Data List
 * @ingroup     sys
 * @brief       Desire Encounter Data (EDL) after windowing and fading
 *
 *
 * TODO:
 *      - try to use half-precision floating point values
 *      - use libfixmath
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef RDL_WINDOW_H
#define RDL_WINDOW_H

#include <inttypes.h>

#include "timex.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Number of windows per EPOCH
 */
#ifndef WINDOWS_PER_EPOCH
#define WINDOWS_PER_EPOCH         15
#endif
/**
 * @brief   Duration of a Window in seconds
 */
#ifndef WINDOW_LENGTH_S
#define WINDOW_LENGTH_S           (120LU)
#endif
/**
 * @brief   Step between windows in seconds
 */
#ifndef WINDOW_STEP_S
#define WINDOW_STEP_S             (60LU)
#endif

/**
 * @brief   Values above this values will be clipped before being averaged
 */
#define RSSI_CLIPPING_THRESH    (0)

/**
 * @brief   Window data
 */
typedef struct rdl_window {
    float avg;              /**< rssi average */
    uint16_t samples;       /**< samples/messages per widnow */
} rdl_window_t;

/*
 * @brief   RDL windows
 */
typedef struct rdl_windows {
    rdl_window_t wins[WINDOWS_PER_EPOCH]; /**< individual windows */
} rdl_windows_t;

/**
 * @brief   Initialize an rdl window list
 *
 * @note    Called by @ref rdl_windows_from_edl_list
 *
 * @param[inout]    wins        the windows
 */
static inline void rdl_windows_init(rdl_windows_t *wins)
{
    memset(wins, '\0', sizeof(rdl_windows_t));
}

/**
 * @brief   Returns windowed rtl from an Encounter Data List (EDL)
 *
 * @param[inout]    wins        RTL windows
 * @param[in]       time        the timestamp that marks the start of an epoch
 * @param[in]       rssi        the received RSSI
 *
 * @return          the found edl_t, NULL otherwise
 */
void rdl_windows_update(rdl_windows_t *wins, float rssi, int16_t time);

/**
 * @brief   Finalize rtl windows by computing the average
 *
 * @param[inout]    wins        the windows
 */
void rdl_windows_finalize(rdl_windows_t *wins);

#ifdef __cplusplus
}
#endif

#endif /* RDL_WINDOW_H */
/** @} */
