/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifndef ED_SHARED_H
#define ED_SHARED_H

#include <stdint.h>

#if __has_include("pepper/config.h")
#include "pepper/config.h"
#else
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Step between windows in seconds
 */
#ifndef MIN_EXPOSURE_TIME_S
#ifdef CONFIG_EPOCH_DURATION_SEC
#define MIN_EXPOSURE_TIME_S     (CONFIG_EPOCH_DURATION_SEC / 3)
#else
#define MIN_EXPOSURE_TIME_S     (10 * 60UL)
#endif
#endif
/**
 * @brief   The minimum request count
 */
#ifndef MIN_REQUEST_COUNT
#define MIN_REQUEST_COUNT       (1U)
#endif

/**
 * @brief   THe maximum distance
 */
#ifndef MAX_DISTANCE_CM
#define MAX_DISTANCE_CM         (200U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* ED_SHARED_H */
