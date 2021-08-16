/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_edho_coap
 * @ingroup     sys
 * @brief       EDHOC CoAP based utilities
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

#define CONFIG_COAP_EDHOC_BUF_SIZE          256
#define CONFIG_COAP_EDHOC_RESOURCE          "/.well-known/edhoc"

typedef struct edhoc_coap_ctx {
    edhoc_ctx_t ctx;
    edhoc_conf_t conf;
    rpk_t rpk;
    cred_id_t id;
    edhoc_cose_key_t key;
    struct tc_sha256_state_struct sha;
} edhoc_coap_ctx_t;

int edhoc_coap_init(edhoc_coap_ctx_t *ctx, edhoc_role_t role, uint8_t* id, size_t id_len);

int edhoc_coap_handshake(edhoc_ctx_t *ctx, sock_udp_ep_t *remote,
                         uint8_t method, uint8_t cypher_suit);

#ifdef __cplusplus
}
#endif

#endif /* EDHOC_COAP_H */
/** @} */
