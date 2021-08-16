/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_coap_initiator
 * @{
 *
 * @file
 * @brief       EDHOC CoAP Intiator Module implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include "net/nanocoap_sock.h"
#include "edhoc/edhoc.h"
#include "edhoc/coap.h"
#include "edhoc/keys.h"
#include "random.h"

#define ENABLE_DEBUG 0
#include "debug.h"

static void print_bstr(const uint8_t *bstr, size_t bstr_len)
{
    for (size_t i = 0; i < bstr_len; i++) {
        if ((i + 1) % 8 == 0) {
            DEBUG("0x%02x \n", bstr[i]);
        }
        else {
            DEBUG("0x%02x ", bstr[i]);
        }
    }
    DEBUG("\n");
}

static ssize_t _build_coap_pkt(coap_pkt_t *pkt,
                               uint8_t *buf, ssize_t buf_len,
                               uint8_t *payload, ssize_t payload_len)
{
    ssize_t len = 0;
    uint8_t token[4];
    uint32_t rand = random_uint32();
    memcpy(&token, &rand, 4);
    /* set pkt buffer */
    pkt->hdr = (coap_hdr_t *)buf;
    /* build header, confirmed message always post */
    ssize_t hdrlen = coap_build_hdr(pkt->hdr, COAP_TYPE_CON, token,
                                    sizeof(token), COAP_METHOD_POST, 1);
    coap_pkt_init(pkt, buf, buf_len, hdrlen);
    coap_opt_add_string(pkt, COAP_OPT_URI_PATH, CONFIG_COAP_EDHOC_RESOURCE, '/');
    coap_opt_add_uint(pkt, COAP_OPT_CONTENT_FORMAT, COAP_FORMAT_OCTET);
    len = coap_opt_finish(pkt, COAP_OPT_FINISH_PAYLOAD);
    /* copy msg payload */
    pkt->payload_len = payload_len;
    memcpy(pkt->payload, payload, payload_len);
    len += pkt->payload_len;
    return len;
}

int edhoc_coap_init(edhoc_coap_ctx_t *ctx, edhoc_role_t role, uint8_t* id, size_t id_len)
{
    /* recover node assigned credentials*/
    const volatile cred_db_entry_t* entry = edhoc_keys_get(id, id_len);
    if (!entry) {
        DEBUG_PUTS("[coap_initiator]: could not find credentials");
        return -1;
    }
    /* reset all structures */
    edhoc_ctx_init(&ctx->ctx);
    edhoc_conf_init(&ctx->conf);
    cred_id_init(&ctx->id);
    cred_rpk_init(&ctx->rpk);
    edhoc_cose_key_init(&ctx->key);

    /* load keys from cbor */
    DEBUG_PUTS("[coap_initiator]: load private authentication key");
    if (edhoc_cose_key_from_cbor(&ctx->key, entry->auth_key, entry->auth_key_len) != 0) {
        return -1;
    }
    DEBUG_PUTS("[coap_initiator]: load and set CBOR RPK");
    if (cred_rpk_from_cbor(&ctx->rpk, entry->cred, entry->cred_len) != 0) {
        return -1;
    }
    /* TODO: the credentials should match the id */
    DEBUG_PUTS("[coap_initonder]: load credential identifier information");
    if (cred_id_from_cbor(&ctx->id, entry->id, entry->id_len) != 0) {
        return -1;
    }
    DEBUG_PUTS("[coap_initiator]: set up EDHOC callbacks and role");
    edhoc_conf_setup_ad_callbacks(&ctx->conf, NULL, NULL, NULL);
    if (edhoc_conf_setup_role(&ctx->conf, role) != 0) {
        return -1;
    }
    DEBUG_PUTS("[coap_initiator]: set up EDHOC credentials");
    if (edhoc_conf_setup_credentials(&ctx->conf, &ctx->key, CRED_TYPE_RPK,
                                     &ctx->rpk, &ctx->id, edhoc_keys_get_cred) != 0) {
        return -1;
    }
    edhoc_ctx_setup(&ctx->ctx, &ctx->conf, &ctx->sha);

    return 0;
}

int edhoc_coap_handshake(edhoc_ctx_t *ctx, sock_udp_ep_t *remote, uint8_t method, uint8_t suite)
{
    /* TODO: reduce this to a single buffer */
    uint8_t buf[CONFIG_COAP_EDHOC_BUF_SIZE] = { 0 };
    coap_pkt_t pkt;
    ssize_t len = 0;
    uint8_t msg[CONFIG_COAP_EDHOC_BUF_SIZE];
    ssize_t msg_len = 0;

    /* correlation value is transport specific */
    corr_t corr = CORR_1_2;

    /* reset state */
    ctx->state = EDHOC_WAITING;

    if ((msg_len = edhoc_create_msg1(ctx, corr, method, suite, msg, sizeof(msg))) > 0) {
        printf("[initiator]: sending msg1 (%d bytes):\n", (int)msg_len);
        print_bstr(msg, msg_len);
        _build_coap_pkt(&pkt, buf, sizeof(buf), msg, msg_len);
        len = nanocoap_request(&pkt, NULL, remote, CONFIG_COAP_EDHOC_BUF_SIZE);
    }
    else {
        puts("[initiator]: failed to create msg1");
        return -1;
    }
    if (len < 0) {
        puts("[initiator]: failed to send msg1");
        return -1;
    }

    printf("[initiator]: received a message (%d bytes):\n", pkt.payload_len);
    print_bstr(pkt.payload, pkt.payload_len);

    if ((msg_len = edhoc_create_msg3(ctx, pkt.payload, pkt.payload_len, msg, sizeof(msg))) > 0) {
        printf("[initiator]: sending msg3 (%d bytes):\n", (int)msg_len);
        print_bstr(msg, msg_len);
        _build_coap_pkt(&pkt, buf, sizeof(buf), msg, msg_len);
        len = nanocoap_request(&pkt, NULL, remote, CONFIG_COAP_EDHOC_BUF_SIZE);
    }
    else {
        puts("[initiator]: failed to create msg3");
        return -1;
    }

    if (edhoc_init_finalize(ctx)) {
        puts("[initiator]: handshake failed");
        return -1;
    }

    puts("[initiator]: handshake successfully completed");
    ctx->state = EDHOC_FINALIZED;

    return 0;
}
