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
 * @brief       Crypto Manager Implementation using HaCL
 *
 * This modules generates a C25519 based key pair. It will also generate
 * Private Encounter Tokens (PET).
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
*/

#include <assert.h>

#include "crypto_manager.h"

#include "kernel_defines.h"

#include "random.h"
#ifndef KRML_NOUINT128
#define KRML_NOUINT128
#endif
#include "Hacl_Curve25519.h"

#define ENABLE_DEBUG    0
#include "debug.h"

int crypto_manager_gen_keypair(crypto_manager_keys_t *keys)
{
    assert(keys);
    random_bytes(keys->sk, C25519_KEY_SIZE);
    /* this makes sure the random bytes are a valid c25519 key */
    keys->sk[0] &= 0xf8;
    keys->sk[31] &= 0x7f;
    keys->sk[31] |= 0x40;
    /* c25519_base_x point */
    uint8_t basepoint[32] = {9};
    Hacl_Curve25519_crypto_scalarmult(keys->pk, keys->sk, basepoint);
    return 0;
}

int crypto_manager_shared_secret(uint8_t *sk, uint8_t *pk, uint8_t *secret)
{
    assert(sk && pk && secret);

    if (IS_ACTIVE(ENABLE_DEBUG)) {
        DEBUG("[crypto_manager]: sk: ");
        for(uint8_t i = 0; i < PET_SIZE; i++) {
            DEBUG("%02x ", sk[i]);
        }
        DEBUG("\n");
    }
    if (IS_ACTIVE(ENABLE_DEBUG)) {
        DEBUG("[crypto_manager]: pk: ");
        for(uint8_t i = 0; i < PET_SIZE; i++) {
            DEBUG("%02x ", pk[i]);
        }
        DEBUG("\n");
    }

    Hacl_Curve25519_crypto_scalarmult(secret, sk, pk);

    if (IS_ACTIVE(ENABLE_DEBUG)) {
        DEBUG("[crypto_manager]: shared secret: ");
        for(uint8_t i = 0; i < PET_SIZE; i++) {
            DEBUG("%02x ", secret[i]);
        }
        DEBUG("\n");
    }

    return 0;
}
