/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_edhoc_coap  EDHOC CoAP Utilities
 * @ingroup     sys
 * @brief       EDHOC CoAP based utilities
 *
 * Utilities to initiate an EDHOC CoAP context and perform simple a key exchange
 * (initiator side)
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef EDHOC_COAP_H
#define EDHOC_COAP_H

#include <inttypes.h>

#include "coap.h"
#include "edhoc/edhoc.h"
#include "tinycrypt/sha256.h"

#include "net/nanocoap_sock.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief       The default BUFFER size for EDHOC messages
 */
#ifndef CONFIG_COAP_EDHOC_BUF_SIZE
#define CONFIG_COAP_EDHOC_BUF_SIZE          256
#endif

/**
 * @brief       The default EDHOC resource
 */
#ifndef CONFIG_COAP_EDHOC_RESOURCE
#define CONFIG_COAP_EDHOC_RESOURCE          "/.well-known/edhoc"
#endif

/**
 * @brief       Utility wrapper to hold all required EDHOC-C strucutres
 */
typedef struct edhoc_coap_ctx {
    edhoc_ctx_t ctx;                    /**< the edhoc context */
    edhoc_conf_t conf;                  /**< edhoc configuration struct */
    rpk_t rpk;                          /**< the credentials */
    cred_id_t id;                       /**< the credential id */
    edhoc_cose_key_t key;               /**< authkey structure */
    struct tc_sha256_state_struct sha;  /**< the hash context*/
} edhoc_coap_ctx_t;


/**
 * @brief       Init an EDHOC CoAP context
 *
 * @param[inout]    ctx     The context to initiate
 * @param[inout]    role    The edhoc role, EDHOC_IS_INITIATOR/EDHOC_IS_RESPONDER
 * @param[in]       id      Pointer to the device ID matching its credentials ID
 * @param[in]       id_len  The ID length
 */
int edhoc_coap_init(edhoc_coap_ctx_t *ctx, edhoc_role_t role, uint8_t *id,
                    size_t id_len);

/**
 * @brief       Perform a EDHOC key exchange agains the remote endpoint
 *
 *
 * @param[in]       ctx     The edhoc context
 * @param[in]       remote  The remote endpoint
 * @param[in]       method  The authentication method, only EDHOC_AUTH_SIGN_SIGN
 *                          currently supported
 * @param[in]       suite   The cipher suit to use, only EDHOC_CIPHER_SUITE_0 is
 *                          supported
 *
 * @returns     0 id succeeded, <0 otherwise
 */
int edhoc_coap_handshake(edhoc_ctx_t *ctx, sock_udp_ep_t *remote,
                         uint8_t method, uint8_t suite);

#ifdef __cplusplus
}
#endif

#endif /* EDHOC_COAP_H */
/** @} */
