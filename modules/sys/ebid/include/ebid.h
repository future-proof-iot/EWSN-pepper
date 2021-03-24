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

#include <stdint.h

#ifdef __cplusplus
extern "C" {
#endif

typedef union
{
    struct {
        uint8_t ebid_1[12];
        uint8_t ebid_2[12];
        uint8_t ebid_3[12];
    } parts;
    uint8_t ebid[32];
    uint8_t pk[32];
} ebid_values_t;

typedef struct {
    ebid_values_t pk;
    uint8_t sk[32];
    uint8_t ebid_4[12];
    bool valid;
} ebid_t;

int ebid_generate(ebid_t* dev);

uint8_t* ebid_get(ebid_t* dev);

int ebid_get_part(ebid_t* dev, uint8_t part, uint8_t* ebid_part);

uint8_t* ebid_get_pk(ebid_t* dev);

uint8_t* ebid_get_sk(ebid_t* dev);

// ebid_reconstruct()

// ebid_compare()

// ebid_add()

#ifdef __cplusplus
}
#endif

#endif /* EBID_H */
/** @} */
