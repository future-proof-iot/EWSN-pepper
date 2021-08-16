/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Certificates and keys for the edhoc example. This values
 *              are taken from the IETF lake WG test vectors, specifically
 *              test vector 34900, see:
 *              https://github.com/lake-wg/edhoc/blob/5ef58e6ee998f4b9aca4b53b35e87375ca356f32/test-vectors-05/vectors.txt
 *
 * @author      Timothy Claeys <timothy.claeys@inria.fr>
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

const volatile cred_db_entry_t* edhoc_keys_get(uint8_t *id_value, size_t id_value_len);

int edhoc_keys_get_cred(const uint8_t *k, size_t k_len, const uint8_t **o, size_t *o_len);

int edhoc_keys_get_auth_key(const uint8_t *k, size_t k_len, const uint8_t **o, size_t *o_len);

int edhoc_keys_get_cred_id(const uint8_t *k, size_t k_len, const uint8_t **o, size_t *o_len);


#ifdef __cplusplus
}
#endif

#endif /* EDHOC_KEYS_H */
