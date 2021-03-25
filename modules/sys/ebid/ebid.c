/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_ebid
 * @{
 *
 * @file
 * @brief       Ephemeral Bluetooth Identifiers Generator implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <string.h>

#include "ebid.h"
#include "crypto_manager.h"
#include "kernel_defines.h"

void ebid_init(ebid_t* ebid)
{
    memset(ebid, 0, sizeof(ebid_t));
}

int ebid_generate(ebid_t* ebid, crypto_manager_keys_t *keys)
{
    ebid->keys = keys;
    int ret = 0;
    if (!IS_ACTIVE(EBID_FIXED_KEYS)) {
        ret = crypto_manager_gen_keypair(ebid->keys);
    }
    if (ret == 0) {
        memcpy(ebid->parts.ebid.u8, ebid->keys->pk, C25519_KEY_SIZE);
    }
    for (uint8_t i = 0; i < C25519_KEY_SIZE; i++) {
        ebid->parts.ebid_4[i] = ebid->parts.ebid.slice.ebid_1[i] ^
                                ebid->parts.ebid.slice.ebid_2[i] ^
                                ebid->parts.ebid.slice.ebid_3[i];
    }
    ebid->status.status |= EBID_HAS_ALL;
    return ret;
}

bool ebid_compare(ebid_t* ebid_1, ebid_t* ebid_2)
{
    return memcmp(ebid_1->parts.ebid.u8, ebid_2->parts.ebid.u8, sizeof(ebid_values_t)) == 0;
}

int ebid_reconstruct(ebid_t* ebid)
{
    uint8_t slices = (ebid->status.status & 0x0F);
    if (slices == EBID_HAS_ALL) {
        return 0;
    }
    else if (__builtin_popcount(slices) != 3) {
        return -1;
    }
    else {
        uint8_t tmp[EBID_SLICE_SIZE];
        for (uint8_t i = 0; i < EBID_SLICE_SIZE; i++) {
            tmp[i] = ebid->parts.ebid.slice.ebid_1[i] ^
                     ebid->parts.ebid.slice.ebid_2[i] ^
                     ebid->parts.ebid.slice.ebid_3[i] ^
                     ebid->parts.ebid_4[i];
        }
        if (!ebid->status.bit.ebid_1_set) {
            memcpy(ebid->parts.ebid.slice.ebid_1, tmp, EBID_SLICE_SIZE);
        }
        if (!ebid->status.bit.ebid_2_set) {
            memcpy(ebid->parts.ebid.slice.ebid_2, tmp, EBID_SLICE_SIZE);
        }
        if (!ebid->status.bit.ebid_3_set) {
            memcpy(ebid->parts.ebid.slice.ebid_3, tmp, EBID_SLICE_SHORT_SIZE);
        }
        if (!ebid->status.bit.ebid_4_set) {
            memcpy(ebid->parts.ebid_4, tmp, EBID_SLICE_SIZE);
        }

        ebid->status.status |= EBID_HAS_ALL;
        return 0;
    }
}
