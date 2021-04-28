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
#include "edl.h"

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
#define WINDOW_LENGTH_S           120LU
#endif
/**
 * @brief   Step between windows in seconds
 */
#ifndef WINDOW_STEP_S
#define WINDOW_STEP_S             60LU
#endif

/**
 * @brief   Window data
 */
typedef struct rdl_window {
    float avg;              /**< rssi average */
    uint16_t samples;       /**< samples/messages per widnow */
} rdl_window_t;

/**
 * @brief   RDL windows
 */
typedef struct rdl_windows
{
    uint32_t timestamp;             /**< timestamp marking start of epoch */
    uint32_t duration;              /**< encounter duration */
    rdl_window_t wins[WINDOWS_PER_EPOCH]; /**< individual windows */
} rdl_windows_t;

/**
 * @brief   Initialize an rdl window list
 *
 * @note    Called by @ref rdl_windows_from_edl_list
 *
 * @param[inout]    wins        the windows
 * @param[in]       timestamp   the timestamp that marks the start of an epoch
 */
static inline void rdl_windows_init(rdl_windows_t* wins, uint32_t timestamp)
{
    memset(wins, '\0', sizeof(rdl_windows_t));
    wins->timestamp = timestamp;
}

/**
 * @brief   Returns windowed rtl from an Encounter Data List (EDL)
 *
 * @param[in]       list        the input data list
 * @param[in]       timestamp   the timestamp that marks the start of an epoch
 * @param[in]       wins        output RTL windows
 *
 * @return          the found edl_t, NULL otherwise
 */
void rdl_windows_from_edl_list(edl_list_t* list, uint32_t timestamp, rdl_windows_t* wins);

#ifdef __cplusplus
}
#endif

#endif /* RDL_WINDOW_H */
/** @} */
