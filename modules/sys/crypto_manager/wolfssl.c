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
 * @brief       Crypto Manager Implementation using WolfCrypt
 *
 * This modules generates a C25519 based key pair. It will also generate
 * Private Encounter Tokens (PET).
 *
 * @author      Anonymous
 *
 * @}
*/

#include <assert.h>

#include "crypto_manager.h"

#include "kernel_defines.h"
#include "wolfssl/wolfcrypt/curve25519.h"

#define ENABLE_DEBUG    0
#include "debug.h"

int crypto_manager_gen_keypair(crypto_manager_keys_t *keys)
{
    assert(keys);
    int ret;

    WC_RNG rng;
    curve25519_key key;

    wc_InitRng(&rng);
    wc_curve25519_init(&key);
    ret = wc_curve25519_make_key(&rng, CURVE25519_KEYSIZE, &key);
    if (ret) {
        goto exit;
    }
    uint32_t key_size = CURVE25519_KEYSIZE;
    ret = wc_curve25519_export_key_raw_ex(&key, keys->sk, (word32 *)&key_size,
                                          keys->pk, (word32 *)&key_size,
                                          EC25519_LITTLE_ENDIAN);
exit:
    wc_FreeRng(&rng);
    return ret;
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

    curve25519_key sec;
    curve25519_key pub;
    int ret = 0;
    size_t secret_len = CURVE25519_KEYSIZE;

    wc_curve25519_init(&sec);
    wc_curve25519_init(&pub);

    ret = wc_curve25519_import_private_ex(sk, CURVE25519_KEYSIZE, &sec,
                                          EC25519_LITTLE_ENDIAN);
    if (ret) {
        DEBUG("[crypto_manager]: failed private key import");
        goto exit;
    }
    ret = wc_curve25519_import_public_ex(pk, CURVE25519_KEYSIZE, &pub,
                                         EC25519_LITTLE_ENDIAN);
    if (ret) {
        DEBUG("[crypto_manager]: failed public key import");
        goto exit;
    }
    ret = wc_curve25519_shared_secret_ex(&sec, &pub, secret, &secret_len,
                                         EC25519_LITTLE_ENDIAN);
exit:
    wc_curve25519_free(&sec);
    wc_curve25519_free(&pub);

    if (IS_ACTIVE(ENABLE_DEBUG)) {
        DEBUG("[crypto_manager]: shared secret: ");
        for(uint8_t i = 0; i < PET_SIZE; i++) {
            DEBUG("%02x ", secret[i]);
        }
        DEBUG("\n");
    }

    return ret;
}
