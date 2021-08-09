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

#include "net/sock/udp.h"
#include "net/gcoap.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Default blocksize for block transactions
 */
#ifndef CONFIG_COAP_UTILS_BLOCK_SIZE
#define CONFIG_COAP_UTILS_BLOCK_SIZE 32
#endif

/**
 * @brief   Context for block transactions
 */
typedef struct coap_block_ctx {
    void (*cleanup)(void *);    /**< cleanup callback */
    void *arg;                  /**< optional argument o pass the the callback */
    size_t last_blknum;       /**< last transmitted blknum */
    uint8_t *data;              /**< pointer to data to transmit */
    size_t len;                 /**< length of data to be transmitted */
    uint8_t format;             /**< media format */
    const char *uri;            /**< uri for block exchange */
} coap_block_ctx_t;

/**
 * @brief coap GET response callback, passes the returned payload and optional arg
 */
typedef void (*coap_get_cb_t)(uint8_t *, uint16_t, void *);

/**
 * @brief   Context for GET request
 */
typedef struct coap_get_ctx {
    coap_get_cb_t cb;           /**< callback upon receiving valid payload */
    void *arg;                  /**< optional argument */
} coap_get_ctx_t;

/**
 * @brief   Initialize a coap_get_ctx_t
 *
 * @param[inout]    ctx     the context to initialize
 * @param[in]       cb      optional callback to handle received payload
 * @param[in]       arg     optional callback argument
 */
static inline void coap_get_ctx_init(coap_get_ctx_t *ctx, coap_get_cb_t cb,
                                     void *arg)
{
    ctx->cb = cb;
    ctx->arg = arg;
}

/**
 * @brief   Initialize a block context
 *
 * @param[inout]    ctx     the block context to initialize
 * @param[in]       data    pointer to the data to transmit, must remain valid
 *                          until the end of the block transaction
 * @param[in]       len     length of data to transmit
 * @param[in]       uri     uri of block exchange destination
 * @param[in]       cleanup optional cleanup callback called upon transcation
 *                          finalization, successfull or not
 * @param[in]       arg     optional callback argument
 * @param[in]       format  media type format
 */
static inline void coap_block_ctx_init(coap_block_ctx_t *ctx, uint8_t *data,
                                       size_t len, const char *uri,
                                       void (*cleanup)(void *), void *arg,
                                       uint8_t format)
{
    memset(ctx, '\0', sizeof(coap_block_ctx_t));
    ctx->len = len;
    ctx->data = data;
    ctx->uri = uri;
    ctx->cleanup = cleanup;
    ctx->arg = arg;
    ctx->format = format;
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
 * @param[inout]    remote      the remote endpoint to initialize
 * @param[in]       handler     the response handler, if NULL a default handler
 *                              is used
 * @param[in]       uri         the destination uri
 * @param[in]       data        the data to be sent
 * @param[in]       len         the length of the data to be sent out
 * @param[in]       format      the media type format to use
 * @param[in]       conf        true if confirmable message, false otherwise
 *
 * @return length of sent packet on success, <0 on error
 */
size_t coap_post(sock_udp_ep_t *remote, gcoap_resp_handler_t handler,
                 const char *uri, uint8_t *data, size_t len, uint8_t format,
                 bool conf);

/**
 * @brief       Perform a CoAP GET
 *
 * @param[inout]    remote      the remote endpoint to initialize
 * @param[in]       ctx         the coap get ctx, can be NULL
 * @param[in]       uri         the destination uri
 * @param[in]       format      the media type format to use
 * @param[in]       conf        true if confirmable message, false otherwise
 *
 *
 * @return length of sent packet on success, <0 on error
 */
size_t coap_get(sock_udp_ep_t *remote, coap_get_ctx_t *ctx, const char *uri,
                uint8_t format, uint8_t type);

/**
 * @brief       Perform a CoAP block POST
 *
 * @param[inout]    remote      the remote endpoint to initialize
 * @param[in]       ctx         the previously initialized block context
 *
 * @retval  0   if successfully started & ended block transaction (single block)
 * @retval  1   if successfully started transaction (multiple blocks)
 * @retval  <0  on error
 */
int coap_block_post(sock_udp_ep_t *remote, coap_block_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* COAP_UTILS_H */
/** @} */
