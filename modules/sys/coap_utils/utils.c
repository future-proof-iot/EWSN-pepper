/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_coap_utils
 * @{
 *
 * @file
 * @brief       CoAP utility functions
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>

#include "net/af.h"
#include "net/ipv6.h"
#include "net/gcoap.h"
#include "net/sock/udp.h"
#include "net/coap.h"
#include "coap/utils.h"
#include "od.h"
#include "fmt.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_DEBUG
#endif
#include "log.h"

/* forward declaration */
static int _do_block_post(coap_pkt_t *pdu, const sock_udp_ep_t *remote,
                          coap_block_ctx_t *ctx);

void _resp_handler(const gcoap_request_memo_t *memo, coap_pkt_t *pdu,
                   const sock_udp_ep_t *remote)
{
    int res = 0;
    void *data = NULL;
    size_t data_len = 0;
    coap_req_ctx_t *context = NULL;

    /* attemp to recover a request context */
    if (memo->context) {
        context = memo->context;
    }

    if (memo->state == GCOAP_MEMO_TIMEOUT) {
        LOG_ERROR("[coap/utils]: timeout for msg ID %02u\n", coap_get_id(pdu));
        res = -ETIMEDOUT;
        goto unlock;
    }
    else if (memo->state == GCOAP_MEMO_ERR) {
        LOG_ERROR("[coap/utils]: error in response\n");
        res = -EBADMSG;
        goto unlock;
    }

    if (coap_get_code_class(pdu) != COAP_CLASS_SUCCESS) {
        LOG_DEBUG("[coap/utils]: unsuccessful response: %1u.%02u\n",
                  coap_get_code_class(pdu), coap_get_code_detail(pdu));
        res = -EBADMSG;
        goto unlock;
    }

    if (coap_get_code_raw(pdu) == COAP_CODE_CONTINUE && context) {
        coap_block_ctx_t *block_context = (coap_block_ctx_t *)context;
        res = _do_block_post(pdu, remote, block_context);

        if (res < 0) {
            goto unlock;
        }
        res = 0;
        return;
    }

    data = pdu->payload;
    data_len = pdu->payload_len;
unlock:
    if (context) {
        if (context->cb) {
            context->cb(res, data, data_len, context->arg);
        }
        mutex_unlock(&context->resp_wait);
    }
    else if (data_len) {
        unsigned content_type = coap_get_content_type(pdu);
        if (content_type == COAP_FORMAT_TEXT
            || content_type == COAP_FORMAT_LINK
            || coap_get_code_class(pdu) == COAP_CLASS_CLIENT_FAILURE
            || coap_get_code_class(pdu) == COAP_CLASS_SERVER_FAILURE) {
            /* Expecting diagnostic payload in failure cases */
            LOG_DEBUG(", %u bytes\n%.*s\n", pdu->payload_len, pdu->payload_len,
                      (char *)pdu->payload);
        }
        else if (LOG_LEVEL == LOG_DEBUG) {
            od_hex_dump(pdu->payload, pdu->payload_len, OD_WIDTH_DEFAULT);
        }
    }
}

int coap_init_remote(sock_udp_ep_t *remote, char *addr_str, uint16_t port)
{
    ipv6_addr_t addr;

    remote->family = AF_INET6;
    remote->port = port;

    LOG_DEBUG("[coap/utils]: remote_ep=([%s]:%d)\n", addr_str, port);

    /* parse for interface */
    char *iface = ipv6_addr_split_iface(addr_str);

    if (!iface) {
        if (gnrc_netif_numof() == 1) {
            remote->netif = (uint16_t)gnrc_netif_iter(NULL)->pid; /* single addr*/
        }
        else {
            remote->netif = SOCK_ADDR_ANY_NETIF;
        }
    }
    else {
        int pid = atoi(iface);
        if (gnrc_netif_get_by_pid(pid) == NULL) {
            LOG_ERROR("[coap/utils]: interface not valid\n");
            return -EINVAL;
        }
        remote->netif = pid;
    }

    /* parse destination address */
    if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
        LOG_ERROR("[coap/utils]: address not valid '%s'\n", addr_str);
        return -EINVAL;
    }
    if ((remote->netif == SOCK_ADDR_ANY_NETIF) &&
        ipv6_addr_is_link_local(&addr)) {
        LOG_ERROR("[coap/utils]: must specify interface for link local target\n");
        return -EINVAL;
    }

    /* set address and port */
    memcpy(&remote->addr.ipv6[0], &addr.u8[0], sizeof(addr.u8));

    return 0;
}

size_t coap_get(sock_udp_ep_t *remote, coap_req_ctx_t *ctx,
                const char *uri, uint8_t format, uint8_t type)
{
    uint8_t buf[CONFIG_GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    size_t msg_len;

    gcoap_req_init(&pdu, &buf[0], CONFIG_GCOAP_PDU_BUF_SIZE, COAP_METHOD_GET,
                   uri);
    coap_hdr_set_type(pdu.hdr, type);
    coap_opt_add_format(&pdu, format);
    msg_len = coap_opt_finish(&pdu, COAP_OPT_FINISH_PAYLOAD);

    if (ctx) {
        if (!mutex_trylock(&ctx->resp_wait)) {
            LOG_ERROR("[coap/utils]: ERROR context is locked\n");
            return -1;
        }
    }
    return gcoap_req_send(buf, msg_len, remote, _resp_handler, ctx);
}

size_t coap_post(sock_udp_ep_t *remote, coap_req_ctx_t *ctx,
                 void *data, size_t data_len,
                 const char *uri, uint8_t format, uint8_t type)
{
    uint8_t buf[CONFIG_GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    size_t msg_len;

    gcoap_req_init(&pdu, &buf[0], CONFIG_GCOAP_PDU_BUF_SIZE, COAP_METHOD_POST,
                   uri);
    coap_hdr_set_type(pdu.hdr, type);
    coap_opt_add_format(&pdu, format);
    msg_len = coap_opt_finish(&pdu, COAP_OPT_FINISH_PAYLOAD);

    if (pdu.payload_len >= data_len) {
        memcpy(pdu.payload, data, data_len);
        msg_len += data_len;
    }
    else {
        LOG_ERROR("[coap/utils]: msg buffer too small\n");
        return -1;
    }
    if (ctx) {
        if (!mutex_trylock(&ctx->resp_wait)) {
            LOG_ERROR("[coap/utils]: ERROR context is locked\n");
            return -1;
        }
    }
    return gcoap_req_send(buf, msg_len, remote, _resp_handler, ctx);
}

static int _do_block_post(coap_pkt_t *pdu, const sock_udp_ep_t *remote,
                          coap_block_ctx_t *ctx)
{
    coap_block_slicer_t slicer;

    LOG_DEBUG("[coap/utils]: blockwise continue, blk=(%d) \n",
              ctx->last_blknum);
    coap_block_slicer_init(&slicer, ctx->last_blknum++,
                           CONFIG_COAP_UTILS_BLOCK_SIZE);

    gcoap_req_init(pdu, (uint8_t *)pdu->hdr, CONFIG_GCOAP_PDU_BUF_SIZE,
                   COAP_METHOD_POST, ctx->uri);
    /* if CONN messagess are used it would required increasing CONFIG_GCOAP_RESEND_BUFS_MAX
       since the used memo is cleared only after the resp_handler exists */
    coap_hdr_set_type(pdu->hdr, COAP_TYPE_NON);
    coap_opt_add_format(pdu, ctx->format);
    coap_opt_add_block1(pdu, &slicer, 1);
    int len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);
    len += coap_blockwise_put_bytes(&slicer, pdu->payload, ctx->data, ctx->data_len);

    int more = coap_block1_finish(&slicer);

    if (slicer.start == 0) {
        LOG_DEBUG("[coap/utils]: sending msg ID %u, %u bytes\n",
                  coap_get_id(pdu), len);
    }

    ssize_t res = gcoap_req_send((uint8_t *)pdu->hdr, len, remote,
                                 _resp_handler, ctx);
    if (res <= 0) {
        LOG_ERROR("[coap/utils]: msg send failed: %d\n", (int)res);
        return res;
    }
    return more;
}

int coap_block_post(sock_udp_ep_t *remote, coap_block_ctx_t *ctx,
                    void *data, size_t data_len,
                    const char *uri, uint8_t format, uint8_t type)
{
    (void) type;
    assert(ctx);

    if (!mutex_trylock(&ctx->req_ctx.resp_wait)) {
        LOG_ERROR("[coap/utils]: ERROR context is locked\n");
        return -1;
    }

    uint8_t buf[CONFIG_GCOAP_PDU_BUF_SIZE] = { 0 };
    coap_pkt_t pdu;
    pdu.hdr = (coap_hdr_t *)buf;
    ctx->data = data;
    ctx->data_len = data_len;
    ctx->uri = uri;
    ctx->format = format;
    ctx->last_blknum = 0;

    return _do_block_post(&pdu, remote, ctx);
}
