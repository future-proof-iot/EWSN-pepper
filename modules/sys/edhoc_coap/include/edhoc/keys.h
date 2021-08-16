/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     sys_edhoc_coap
 *
 * @file
 * @brief       This header describes utilities for a credential database, this
 *              allows for multiple devices to be provided and access the same
 *              credentials easily.
 *
 *              Credentials should be CBOR encoded to match the EDHOC-C keys
 *              parsing API.
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#ifndef EDHOC_KEYS_H
#define EDHOC_KEYS_H

#include <inttypes.h>

#include "kernel_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     Credential database  entry
 */
typedef struct {
    const uint8_t *auth_key;    /**< authentication key */
    size_t auth_key_len;        /**< authentication key len */
    const uint8_t *id;          /**< credential id pointer */
    size_t id_len;              /**< credential id length */
    const uint8_t *id_value;    /**< credential id value pointer */
    size_t id_value_len;        /**< credential id valuelength */
    const uint8_t *cred;        /**< credential pointer */
    size_t cred_len;            /**< credential length */
} cred_db_entry_t;

/**
 * @brief   Retrieves a credential database entry based on an Key ID
 *
 * @param[in]   id_value        Pointer to the key ID
 * @param[in]   id_value_len    Length of the key ID
 *
 * @return      pointer to the entry, NULL if not found
 */
const volatile cred_db_entry_t *edhoc_keys_get(uint8_t *id_value, size_t id_value_len);

/**
 * @brief   Returns CBOR encoded credentials (RPK) matching the provided key id
 *
 * @param[in]       k           The key to match
 * @param[in]       k_len       The length of the key id to match
 * @param[inout]    o           Pointer to the credential buffer, NULL if not found
 * @param[inout]    o_len       Length of the matched credential, 0 if not found
 *
 * @returns         0 if found, <0 otherwise
 */
int edhoc_keys_get_cred(const uint8_t *k, size_t k_len, const uint8_t **o, size_t *o_len);


/**
 * @brief   Returns CBOR encoded authentication key matching the provided key id
 *
 * @param[in]       k           The key to match
 * @param[in]       k_len       The length of the key id to match
 * @param[inout]    o           Pointer to the credential buffer, NULL if not found
 * @param[inout]    o_len       Length of the matched credential, 0 if not found
 *
 * @returns         0 if found, <0 otherwise
 */
int edhoc_keys_get_auth_key(const uint8_t *k, size_t k_len, const uint8_t **o, size_t *o_len);

/**
 * @brief   Returns CBOR encoded Key ID matching the provided key id
 *
 * @param[in]       k           The key to match
 * @param[in]       k_len       The length of the key id to match
 * @param[inout]    o           Pointer to the credential buffer, NULL if not found
 * @param[inout]    o_len       Length of the matched credential, 0 if not found
 *
 * @returns         0 if found, <0 otherwise
 */
int edhoc_keys_get_cred_id(const uint8_t *k, size_t k_len, const uint8_t **o, size_t *o_len);

#ifdef __cplusplus
}
#endif

#endif /* EDHOC_KEYS_H */
