/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_coap_util CoAP utilities
 * @ingroup     sys
 *
 * @brief       CoAP utility functions
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef COAP_UTILS_H
#define COAP_UTILS_H

#include <inttypes.h>
#include <string.h>
#include <stdbool.h>

#include "mutex.h"
#include "net/sock/udp.h"
#include "net/gcoap.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Default blocksize for block transactions
 */
#ifndef CONFIG_COAP_UTILS_BLOCK_SIZE
#define CONFIG_COAP_UTILS_BLOCK_SIZE    64
#endif

/**
 * @brief CoAP response callback, passes the returned payload and optional arg,
 *        the payload can be NULL.
 */
typedef void (*coap_req_cb_t)(int, void *, size_t, void *);

/**
 * @brief CoAP request context
 */
typedef struct coap_req_ctx {
    mutex_t resp_wait;          /**< locked when context is in use */
    coap_req_cb_t cb;           /**< req completion callback */
    void *arg;                  /**< optional argument */
} coap_req_ctx_t;

/**
 * @brief   Context for block transactions
 */
typedef struct coap_block_ctx {
    coap_req_ctx_t req_ctx;     /**< CoAP request context */
    size_t last_blknum;         /**< last transmitted blknum */
    void *data;                 /**< pointer to data to transmit */
    size_t data_len;            /**< length of data to be transmitted */
    uint8_t format;             /**< media format */
    const char *uri;            /**< uri for block exchange */
} coap_block_ctx_t;

/**
 * @brief   Initialize a coap_get_ctx_t
 *
 * @param[inout]    ctx     the context to initialize
 * @param[in]       cb      optional callback to handle received payload
 * @param[in]       arg     optional callback argument
 */
static inline void coap_req_ctx_init(coap_req_ctx_t *ctx, coap_req_cb_t cb,
                                     void *arg)
{
    mutex_init(&ctx->resp_wait);
    ctx->cb = cb;
    ctx->arg = arg;
}

/**
 * @brief   Checks if the CoAP request context is free
 *
 * @param[inout]    ctx     the context to initialize
 *
 * @return  true if free, false otherwise
 */
static inline bool coap_req_ctx_is_free(coap_req_ctx_t *ctx)
{
    if (mutex_trylock(&ctx->resp_wait)) {
        mutex_unlock(&ctx->resp_wait);
        return true;
    }
    else {
        return false;
    }
}

/**
 * @brief       Initialize a remote end points
 *
 * @param[inout]    remote      the remote endpoint to initialize
 * @param[in]       addr_str    the remote endpoint ipv6 addr
 * @param[in]       port        the remote port
 */
int coap_init_remote(sock_udp_ep_t *remote, char *addr_str, uint16_t port);

/**
 * @brief       Perform a CoAP POST
 *
 * @param[inout]    remote      the remote endpoint
 * @param[in]       ctx         the coap get ctx, can be NULL
 * @param[in]       uri         the destination uri
 * @param[in]       data        the data to be sent
 * @param[in]       data_len    the length of the data to be sent out
 * @param[in]       format      the media type format to use
 * @param[in]       type        the message type
 *
 * @return length of sent packet on success, <0 on error
 */
int coap_post(sock_udp_ep_t *remote, coap_req_ctx_t *ctx,
                 void *data, size_t data_len,
                 const char *uri, uint8_t format, uint8_t type);

/**
 * @brief       Perform a CoAP GET
 *
 * @param[inout]    remote      the remote endpoint
 * @param[in]       ctx         the coap get ctx, can be NULL
 * @param[in]       uri         the destination uri
 * @param[in]       format      the media type format to use
 * @param[in]       type        the message type
 *
 *
 * @return length of sent packet on success, <0 on error
 */
int coap_get(sock_udp_ep_t *remote, coap_req_ctx_t *ctx,
                const char *uri, uint8_t format, uint8_t type);

/**
 * @brief       Perform a CoAP block POST
 *
 * @param[inout]    remote      the remote endpoint
 * @param[in]       ctx         the coap block ctx, cant be NULL
 * @param[in]       uri         the destination uri
 * @param[in]       data        the data to be sent
 * @param[in]       data_len    the length of the data to be sent out
 * @param[in]       format      the media type format to use
 * @param[in]       type        the message type
 *
 * @retval  0   if successfully started & ended block transaction (single block)
 * @retval  1   if successfully started transaction (multiple blocks)
 * @retval  <0  on error
 */
int coap_block_post(sock_udp_ep_t *remote, coap_block_ctx_t *ctx,
                    void *data, size_t data_len,
                    const char *uri, uint8_t format, uint8_t type);

#ifdef __cplusplus
}
#endif

#endif /* COAP_UTILS_H */
/** @} */
