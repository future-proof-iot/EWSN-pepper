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
 * @brief       Crypto Manager implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include "crypto_manager.h"

#include "hashes/sha256.h"
#include "wolfssl/wolfcrypt/curve25519.h"

#define ENABLE_DEBUG    1
#include "debug.h"

int crypto_manager_gen_keypair(crypto_manager_keys_t *keys)
{
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
    ret = wc_curve25519_export_key_raw(&key, keys->sk, (word32 *)&key_size,
                                       keys->pk, (word32 *)&key_size);
exit:
    wc_FreeRng(&rng);
    return ret;
}

int _gen_shared_secret(crypto_manager_keys_t *sk, crypto_manager_keys_t *pk,
                       uint8_t *secret, size_t secret_len)
{
    curve25519_key sec;
    curve25519_key pub;
    int ret = 0;

    wc_curve25519_init(&sec);
    wc_curve25519_init(&pub);

    ret = wc_curve25519_import_private_ex(sk->sk, CURVE25519_KEYSIZE, &sec,
                                          EC25519_LITTLE_ENDIAN);
    if (ret) {
        goto exit;
    }
    ret = wc_curve25519_import_public_ex(pk->pk, CURVE25519_KEYSIZE, &pub,
                                         EC25519_LITTLE_ENDIAN);
    if (ret) {
        goto exit;
    }
    ret = wc_curve25519_shared_secret_ex(&pub, &pub, secret, &secret_len,
                                         EC25519_LITTLE_ENDIAN);

exit:
    wc_curve25519_free(&sec);
    wc_curve25519_free(&pub);

    return ret;
}

int crypto_manager_gen_pet(crypto_manager_keys_t *sk, crypto_manager_keys_t *pk,
                           uint8_t *prefix, uint8_t *pet)
{
    uint8_t buf[SHARED_SECRET_SIZE];
    uint8_t secret[SHARED_SECRET_SIZE];

    if (_gen_shared_secret(sk, pk, secret, SHARED_SECRET_SIZE)) {
        return -1;
    }

    for (uint8_t i = 0; i < SHARED_SECRET_SIZE; i++) {
        buf[i] =  secret[i] + prefix[i];
    }

    sha256_context_t sha256;
    sha256_init(&sha256);
    sha256_update(&sha256, buf, SHARED_SECRET_SIZE);
    sha256_final(&sha256, pet);

    return 0;
}