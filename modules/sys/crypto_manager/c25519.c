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
 * @brief       Crypto Manager Implementation using C25519
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
#include "c25519.h"
#include "random.h"

#define ENABLE_DEBUG    0
#include "debug.h"

int crypto_manager_gen_keypair(crypto_manager_keys_t *keys)
{
    assert(keys);
    random_bytes(keys->sk, C25519_KEY_SIZE);
    c25519_prepare(keys->sk);
    c25519_smult(keys->pk, c25519_base_x, keys->sk);
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

    c25519_smult(secret, pk, sk);

    if (IS_ACTIVE(ENABLE_DEBUG)) {
        DEBUG("[crypto_manager]: shared secret: ");
        for(uint8_t i = 0; i < PET_SIZE; i++) {
            DEBUG("%02x ", secret[i]);
        }
        DEBUG("\n");
    }

    return 0;
}
