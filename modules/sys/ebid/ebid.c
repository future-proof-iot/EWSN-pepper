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
 * @author      Anonymous
 *
 * @}
 */

#include <string.h>

#include "ebid.h"
#include "crypto_manager.h"
#include "kernel_defines.h"
#include "random.h"

void ebid_init(ebid_t* ebid)
{
    memset(ebid, 0, sizeof(ebid_t));
}

void ebid_generate(ebid_t* ebid, crypto_manager_keys_t *keys)
{
    ebid_generate_from_pk(ebid, keys->pk);
}

void ebid_generate_from_pk(ebid_t* ebid, uint8_t* pk)
{
    memcpy(ebid->parts.ebid.u8, pk, C25519_KEY_SIZE);
    for (uint8_t i = 0; i < EBID_SLICE_SIZE_LONG; i++) {
        ebid->parts.ebid_xor[i] = ebid->parts.ebid.slice.ebid_1[i] ^
                                  ebid->parts.ebid.slice.ebid_2[i] ^
                                  ebid->parts.ebid.slice.ebid_3_padded[i];
    }
    ebid->status.status |= EBID_HAS_ALL;
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
        uint8_t tmp[EBID_SLICE_SIZE_LONG];
        for (uint8_t i = 0; i < EBID_SLICE_SIZE_LONG; i++) {
            tmp[i] = ebid->parts.ebid.slice.ebid_1[i] ^
                     ebid->parts.ebid.slice.ebid_2[i] ^
                     ebid->parts.ebid.slice.ebid_3_padded[i] ^
                     ebid->parts.ebid_xor[i];
        }
        if (!ebid->status.bit.ebid_1_set) {
            memcpy(ebid->parts.ebid.slice.ebid_1, tmp, EBID_SLICE_SIZE_LONG);
        }
        if (!ebid->status.bit.ebid_2_set) {
            memcpy(ebid->parts.ebid.slice.ebid_2, tmp, EBID_SLICE_SIZE_LONG);
        }
        if (!ebid->status.bit.ebid_3_set) {
            memcpy(ebid->parts.ebid.slice.ebid_3, tmp, EBID_SLICE_SIZE_SHORT);
        }
        if (!ebid->status.bit.ebid_xor_set) {
            memcpy(ebid->parts.ebid_xor, tmp, EBID_SLICE_SIZE_LONG);
        }

        ebid->status.status |= EBID_HAS_ALL;
        return 0;
    }
}
