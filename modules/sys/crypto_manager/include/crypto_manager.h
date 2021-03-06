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
 * @brief       DESIRE Crypto Manager
 *
 * This modules allows generating elliptic curve Curve25519 based public/secret
 * key pair. It can also generate Private Encounter Tokens from a give public/secret
 * key pair and an encountered EBID (which is nothing else than a public key).
 *
 * @see <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/bind.html">
 *          DESIRE: Leveraging the best of centralized and decentralized contact tracing systems
 *      </a>
 *
 * A public/private key pair can ve generated from a preallocated @ref crypto_manager_keys_t
 * structure by calling @ref crypto_manager_gen_keypair.
 *
 * Once EBIDs are received the Private Encounter Tokens can be generated by calling
 * @ref crypto_manager_gen_pets with the received EBID and the host public/private
 * key pair for that encounter.
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

/**
 * @brief   c25519 public and secret key size
 */
#define     C25519_KEY_SIZE         (32U)
/**
 * @brief   Private Encounter Tokens (PET) size
 */
#define     PET_SIZE                (32U)

/**
 * @brief   Public and Secret key pair
 */
typedef struct {
    uint8_t pk[C25519_KEY_SIZE];    /**< public key */
    uint8_t sk[C25519_KEY_SIZE];    /**< secret key */
} crypto_manager_keys_t;

/**
 * @brief   Private Encounter Tokens (PET)
 */
typedef struct {
    uint8_t rt[PET_SIZE];    /**< request token */
    uint8_t et[PET_SIZE];    /**< exposure token */
} pet_t;

/**
 * @brief       Generate a public/secret key pair
 *
 * @param[inout]    keys    Preallocated keys structure to hold generated keys
 *
 * @return      0 on success, -1 otherwise
 */
int crypto_manager_gen_keypair(crypto_manager_keys_t* keys);

/**
 * @brief       Compute shared secret key
 *
 * This function computes a shared secret key given a secret private key and a received public key.
 * It stores the generated secret key in the 'secret' out buffer.
 *
 * @param[in]       sk      32 bytes secret key
 * @param[in]       pk      32 bytes public key (pk != keys->pk)
 * @param[in]       secret  Pointer to a buffer in which to store the 32 byte computed secret key.
 *
 * @return      0 on success, -1 otherwise
 */
int crypto_manager_shared_secret(uint8_t *sk, uint8_t *pk, uint8_t *secret);

/**
 * @brief       Generate a Privacy Encounter Token
 *
 * This function will generate a shared secret from the keys secret key (keys->sk)
 * and the input public key. This result is then ored with the prefix buffer before
 * generating a sha256 digest from it, which is returned as the generated PET.
 *
 * -> H ( ???prefix??? | g^(keys->sk * pk) )
 *
 * @param[in]       keys    Already generated key pair pointer
 * @param[in]       pk      32 bytes public key (pk != keys->pk)
 * @param[in]       prefix  32 bytes buffer to add to shared secret before hashing
 * @param[inout]    pet     Preallocated buffer to hold the Private Encounter
 *                          Token
 *
 * @return      0 on success, -1 otherwise
 */
int crypto_manager_gen_pet(crypto_manager_keys_t *keys, uint8_t *pk,
                           const uint8_t prefix, uint8_t *pet);

/**
 * @brief       Generate a Request Token and an Encounter Token
 *
 * @param[in]       keys    Already generated key pair pointer
 * @param[in]       ebid    32 bytes sized buffer holding a received ebid
 * @param[inout]    pet     Preallocated buffer to hold generated Request and
 *                          Encounter token.
 *
 * @return      0 on success, -1 otherwise
 */
int crypto_manager_gen_pets(crypto_manager_keys_t *keys, uint8_t *ebid,
                            pet_t* pet);

/**
 * @brief       Check wether array a is greater than b
 *
 * @param[inout]     a      The array to comapre
 * @param[inout]     b      The array to compare against
 *
 * @retval   0  if a >  b
 * @retval   1  if b >  a
 * @retval  -1 if a == b
 */
int8_t array_a_greater_than_b(uint8_t *a, uint8_t *b);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_MANAGER_H */
/** @} */
