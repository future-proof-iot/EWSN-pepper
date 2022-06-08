/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_crypto_manager
 * @{
 *
 * @file
 * @brief       Crypto Manager Common implementation
 *
 * @author      Anonymous
 *
 * @}
 */

#include <assert.h>

#include "crypto_manager.h"
#include "kernel_defines.h"
#include "hashes/sha256.h"

#define ENABLE_DEBUG    0
#include "debug.h"

int8_t array_a_greater_than_b(uint8_t *a, uint8_t *b)
{
    for (uint8_t i = 0; i < C25519_KEY_SIZE; i++) {
        if (a[i] > b[i]) {
            return 1;
        }
        else if (a[i] < b[i]) {
            return 0;
        }
    }
    return -1;
}

int crypto_manager_gen_pet(crypto_manager_keys_t *keys, uint8_t *pk,
                           const uint8_t prefix, uint8_t *pet)
{
    assert(keys && pk && prefix && pet);
    uint8_t secret[PET_SIZE] = {0};
    /* clear pet */
    memset(pet, 0, PET_SIZE);

    if (crypto_manager_shared_secret(keys->sk, pk, secret)) {
        DEBUG("[crypto_manager]: failed secret generation");
        return -1;
    }

    /* calculate hash of prefix | secret */
    sha256_context_t sha256;
    sha256_init(&sha256);
    sha256_update(&sha256, &prefix, 1);
    sha256_update(&sha256, secret, PET_SIZE);
    sha256_final(&sha256, pet);

    return 0;
}

int crypto_manager_gen_pets(crypto_manager_keys_t *keys, uint8_t *ebid,
                            pet_t* pet)
{
    assert(keys && ebid && pet);

    int8_t pk_gt_ebid = array_a_greater_than_b(keys->pk, ebid);
    if (pk_gt_ebid == -1) {
        return -1;
    } else if (pk_gt_ebid == 0) {
        crypto_manager_gen_pet(keys, ebid, 0x02, pet->et);
        crypto_manager_gen_pet(keys, ebid, 0x01, pet->rt);
    } else {
        crypto_manager_gen_pet(keys, ebid, 0x01, pet->et);
        crypto_manager_gen_pet(keys, ebid, 0x02, pet->rt);
    }
    return 0;
}

