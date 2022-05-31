/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifndef BPF_ED_H
#define BPF_ED_H

#include <stdint.h>
#include "ed_shared.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   ed context for femto-container/bpf
 */
typedef struct {
    uint16_t time;      /**< exposure duration in s */
    uint16_t distance;  /**< average distance in cm */
    uint16_t req_count; /**< successful TWR request count */
} ed_uwb_bpf_ctx_t;

#ifdef __cplusplus
}
#endif
#endif /* BPF_ED */
