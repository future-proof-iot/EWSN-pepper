/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_crypto_manager Crypto Manager
 * @ingroup     sys
 * @brief       Desire Crypto Manager
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef CRYPTO_MANAGER_H
#define CRYPTO_MANAGER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define     C25519_KEY_SIZE         (32U)
#define     SHARED_SECRET_SIZE      (32U)

typedef struct {
    uint8_t pk[C25519_KEY_SIZE];
    uint8_t sk[C25519_KEY_SIZE];
} crypto_manager_keys_t;

int crypto_manager_gen_keypair(crypto_manager_keys_t* keys);

int crypto_manager_gen_pet(crypto_manager_keys_t* sk, crypto_manager_keys_t* pk,
                           uint8_t* prefix, uint8_t* pet);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_MANAGER_H */
/** @} */
