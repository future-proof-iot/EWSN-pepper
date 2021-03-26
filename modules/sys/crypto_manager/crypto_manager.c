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
 * @brief       Crypto Manager Implementation
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

#include "hashes/sha256.h"
#include "wolfssl/wolfcrypt/curve25519.h"

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
    ret = wc_curve25519_export_key_raw(&key, keys->sk, (word32 *)&key_size,
                                       keys->pk, (word32 *)&key_size);
exit:
    wc_FreeRng(&rng);
    return ret;
}

int _gen_shared_secret(uint8_t *sk, uint8_t *pk, uint8_t *secret, size_t secret_len)
{
    curve25519_key sec;
    curve25519_key pub;
    int ret = 0;

    wc_curve25519_init(&sec);
    wc_curve25519_init(&pub);

    ret = wc_curve25519_import_private_ex(sk, CURVE25519_KEYSIZE, &sec,
                                          EC25519_LITTLE_ENDIAN);
    if (ret) {
        goto exit;
    }
    ret = wc_curve25519_import_public_ex(pk, CURVE25519_KEYSIZE, &pub,
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

int crypto_manager_gen_pet(crypto_manager_keys_t *keys, uint8_t *pk,
                           const uint8_t *prefix, uint8_t *pet)
{
    assert(keys && pk && prefix && pet);

    uint8_t buf[PET_SIZE];
    uint8_t secret[PET_SIZE];

    if (_gen_shared_secret(keys->sk, pk, secret, PET_SIZE)) {
        return -1;
    }

    for (uint8_t i = 0; i < PET_SIZE; i++) {
        buf[i] =  secret[i] + prefix[i];
    }

    sha256_context_t sha256;
    sha256_init(&sha256);
    sha256_update(&sha256, buf, PET_SIZE);
    sha256_final(&sha256, pet);

    return 0;
}

int crypto_manager_gen_pets(crypto_manager_keys_t *keys, uint8_t *ebid,
                            pet_t* pet)
{
    assert(keys && ebid && pet);

    static const uint8_t ones[] = {
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    };
    static const uint8_t twos[] = {
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    };

    int8_t pk_gt_ebid = -1;
    for (uint8_t i = 0; i < CURVE25519_KEYSIZE; i ++) {
        if (keys->pk[i] > ebid[i]) {
            pk_gt_ebid = 1;
            break;
        } else if (keys->pk[i] < ebid[i]) {
            pk_gt_ebid = 0;
            break;
        }
    }
    if (pk_gt_ebid == -1) {
        return -1;
    } else if (pk_gt_ebid == 0) {
        crypto_manager_gen_pet(keys, ebid, twos, pet->et);
        crypto_manager_gen_pet(keys, ebid, ones, pet->rt);
    } else {
        crypto_manager_gen_pet(keys, ebid, ones, pet->et);
        crypto_manager_gen_pet(keys, ebid, twos, pet->rt);
    }
    return 0;
}

