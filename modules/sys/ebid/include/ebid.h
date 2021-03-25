/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_ebid Ephemeral Bluetooth Identifiers Generator
 * @ingroup     sys
 * @brief       C25519 based Ephemeral Bluetooth Identifiers Generator
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef EBID_H
#define EBID_H

#include <stdint.h>
#include <stdbool.h>

#include "crypto_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EBID_SLICE_SIZE         12
#define EBID_SLICE_SHORT_SIZE   8
#define EBID_SIZE               C25519_KEY_SIZE

#define EBID_HAS_SLICE_1          (1 << 0)
#define EBID_HAS_SLICE_2          (1 << 1)
#define EBID_HAS_SLICE_3          (1 << 2)
#define EBID_HAS_SLICE_4          (1 << 3)
#define EBID_HAS_ALL              (EBID_HAS_SLICE_1 | EBID_HAS_SLICE_2 | \
                                   EBID_HAS_SLICE_3 | EBID_HAS_SLICE_4)
#define EBID_VALID                (EBID_HAS_ALL)

typedef union __attribute__((packed)) {
    struct __attribute__((packed)) {
        uint8_t ebid_1[EBID_SLICE_SIZE];
        uint8_t ebid_2[EBID_SLICE_SIZE];
        uint8_t ebid_3[EBID_SLICE_SIZE];
    } slice;
    uint8_t u8[EBID_SIZE];
} ebid_values_t;

typedef union __attribute__((packed)) {
    uint8_t status;
    struct {
        uint8_t ebid_1_set:1;
        uint8_t ebid_2_set:1;
        uint8_t ebid_3_set:1;
        uint8_t ebid_4_set:1;
    } bit;
} ebid_status_t;

typedef struct {
    ebid_values_t ebid;
    uint8_t ebid_4[EBID_SLICE_SIZE];
} ebid_parts_t;

typedef struct __attribute__((packed)) {
    crypto_manager_keys_t* keys;
    ebid_parts_t parts;
    ebid_status_t status;
} ebid_t;

void ebid_init(ebid_t* ebid);

int ebid_generate(ebid_t* ebid, crypto_manager_keys_t *keys);

static inline uint8_t* ebid_get(ebid_t* ebid)
{
    return ebid->parts.ebid.u8;
}

static inline uint8_t* ebid_get_part1(ebid_t* ebid)
{
    return ebid->parts.ebid.slice.ebid_1;
}

static inline uint8_t* ebid_get_part2(ebid_t* ebid)
{
    return ebid->parts.ebid.slice.ebid_2;
}

static inline uint8_t* ebid_get_part3(ebid_t* ebid)
{
    return ebid->parts.ebid.slice.ebid_3;
}

static inline uint8_t* ebid_get_part4(ebid_t* ebid)
{
    return ebid->parts.ebid_4;
}

static inline void ebid_set_part1(ebid_t* ebid, const uint8_t* ebid_1)
{
    memcpy(ebid->parts.ebid.slice.ebid_1, ebid_1, EBID_SLICE_SIZE);
    ebid->status.status |= EBID_HAS_SLICE_1;
}

static inline void ebid_set_part2(ebid_t* ebid, const uint8_t* ebid_2)
{
    memcpy(ebid->parts.ebid.slice.ebid_2, ebid_2, EBID_SLICE_SIZE);
    ebid->status.status |= EBID_HAS_SLICE_2;
}

static inline void ebid_set_part3(ebid_t* ebid, const uint8_t* ebid_3)
{
    memcpy(ebid->parts.ebid.slice.ebid_3, ebid_3, EBID_SLICE_SHORT_SIZE);
    ebid->status.status |= EBID_HAS_SLICE_3;
}

static inline void ebid_set_part4(ebid_t* ebid, const uint8_t* ebid_4)
{
    memcpy(ebid->parts.ebid_4, ebid_4, EBID_SLICE_SIZE);
    ebid->status.status |= EBID_HAS_SLICE_4;
}

int ebid_set_part(ebid_t* ebid, uint8_t part, uint8_t* ebid_part);

uint8_t* ebid_get_pk(ebid_t* ebid);

uint8_t* ebid_get_sk(ebid_t* ebid);

int ebid_reconstruct(ebid_t* ebid);

bool ebid_compare(ebid_t* ebid_1, ebid_t* ebid_2);

#ifdef __cplusplus
}
#endif

#endif /* EBID_H */
/** @} */
