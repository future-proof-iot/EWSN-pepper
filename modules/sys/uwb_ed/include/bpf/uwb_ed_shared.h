/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifndef BPF_UWB_ED_SHARED_H
#define BPF_UWB_ED_SHARED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Step between windows in seconds
 */
#ifndef MIN_EXPOSURE_TIME_S
#define MIN_EXPOSURE_TIME_S     (10 * 60LU)
#endif

/**
 * @brief   Step between windows in seconds
 */
#ifndef MAX_DISTANCE_CM
#define MAX_DISTANCE_CM         (200U)
#endif

typedef struct {
    uint16_t time;
    uint16_t distance;
} bpf_uwb_ed_ctx_t;

#ifdef __cplusplus
}
#endif
#endif /* BPF_UWB_ED_SHARED_H */
