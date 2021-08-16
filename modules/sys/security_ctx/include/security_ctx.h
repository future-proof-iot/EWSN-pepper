/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_security_ctx Crypto Context
 * @ingroup     sys
 * @brief       Crypto Context for EDHOC derived keys
 *
 * A poor mans OSCORE inspired security context
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef SECURITY_CTX_H
#define SECURITY_CTX_H

#include <stdbool.h>
#include <inttypes.h>

#if IS_USED(MODULE_EDHOC_COAP)
#include "edhoc/coap.h"
#include "net/sock.h"
#include "random.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Security context key length
 */
#define     SECURITY_CTX_KEY_LEN          (16U)
/**
 * @brief   Security context key common IV length
 */
#define     SECURITY_CTX_COMMON_IV_LEN    (13)
/**
 * @brief   Security context nonce length
 */
#define     SECURITY_CTX_NONCE_LEN        (13)
/**
 * @brief   Security context ID max length
 */
#define     SECURITY_CTX_ID_MAX_LEN       (6)
/**
 * @brief   Security context EDHOC exporter salt label
 */
#define     SECURITY_CTX_SALT_LABEL     "OSCORE Master Salt"
/**
 * @brief   Security context EDHOC exporter secret label
 */
#define     SECURITY_CTX_SECRET_LABEL   "OSCORE Master Secret"

/**
 * @brief   Security context structure
 */
typedef struct security_ctx {
    uint8_t send_ctx_key[SECURITY_CTX_KEY_LEN];     /**< the send ctx encryption key */
    uint8_t recv_ctx_key[SECURITY_CTX_KEY_LEN];     /**< the recv ctx decryption key */
    uint8_t common_iv[SECURITY_CTX_COMMON_IV_LEN];  /**< the common IV */
    size_t send_id_len;                             /**< the send ctx id len */
    size_t recv_id_len;                             /**< the recv context id len */
    uint16_t seqnr;                                 /**< the seqnr, increased on
                                                          every send */
    uint8_t* recv_id;                               /**< the recv context id */
    uint8_t* send_id;                               /**< the send context id */
    bool valid;                                     /**< current context validity */
} security_ctx_t;

/**
 * @brief   Intiate a security context
 *
 * @param[inout]    ctx         the ctx to init
 * @param[in]       send_id     the send context id
 * @param[in]       send_id_len the send context id length
 * @param[in]       recv_id     the recv context id
 * @param[in]       recv_id_len the recv context id length
 */
void security_ctx_init(security_ctx_t *ctx, uint8_t *send_id,
                       size_t send_id_len, uint8_t *recv_id,
                       size_t recv_id_len);

/**
 * @brief   Generates send and recv context keys
 *
 * @param[in]       ctx         the ctx
 * @param[in]       salt        the salt
 * @param[in]       salt_len    the salt length
 * @param[in]       secret      the secret
 * @param[in]       secret_len  the secret length
 *
 * @returns     0 if succeeded, <0 otherwise
 */
int security_ctx_key_gen(security_ctx_t *ctx, uint8_t *salt, size_t salt_len,
                         uint8_t *secret, size_t secret_len);

/**
 * @brief   Generates a nonce, exposed only for testing
 * @internal
 *
 * @param[in]       ctx         the ctx
 * @param[in]       ctx_id      the ctx id
 * @param[in]       ctx_id_len  the ctx id length
 * @param[in]       secret      the secret
 * @param[inout]    nonce       nonce buffer, must be SECURITY_CTX_NONCE_LEN
 *
 * @returns     0 if succeeded, <0 otherwise
 */
void security_ctx_gen_nonce(security_ctx_t *ctx, uint8_t *ctx_id,
                            size_t ctx_id_len, uint8_t *nonce);

/**
 * @brief   Encode data
 *
 * @param[in]       ctx         the ctx
 * @param[in]       in          data to encode
 * @param[in]       in_len      length of data to encode
 * @param[inout]    buf         encoding buffer
 * @param[in]       buf_len     length of encoding buffer
 * @param[inout]    out         pointer to encoded data location
 *
 * @return          length of encoded data, <0 on error
 */
int security_ctx_encode(security_ctx_t *ctx, uint8_t *data, size_t data_len,
                        uint8_t *buf, size_t buf_len, uint8_t **out);

/**
 * @brief   Decode data
 *
 * @param[in]       ctx         the ctx
 * @param[in]       in          data to decode
 * @param[in]       in_len      length of data to decode
 * @param[inout]    buf         decoding buffer for COSE intermediates
 * @param[in]       buf_len     length of decoding
 * @param[inout]    out         output buffer
 * @param[inout]    out_len     output buffer length, length of decoded data
 *
 * @return          length of encoded data, <0 on error
 */
int security_ctx_decode(security_ctx_t *ctx, uint8_t *in, size_t in_len,
                        uint8_t *buf, size_t buf_len, uint8_t *out, size_t *olen);

#if IS_USED(MODULE_EDHOC_COAP)
/**
 * @brief   Utility to initiate a security context via salt/secret deriver from
 *          an EDHOC key exchange
 *
 * @param[inout]    ctx     the security context
 * @param[in]       e_ctx   the edhoc context
 * @param[in]       remote  the remote endpoint
 *
 * @returns     0 if succeeded, <0 otherwise
 */
int security_ctx_edhoc_handshake(security_ctx_t *ctx, edhoc_ctx_t *e_ctx,
                                 sock_udp_ep_t *remote);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_CTX_H */
/** @} */
