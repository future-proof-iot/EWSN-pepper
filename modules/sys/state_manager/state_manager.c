/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_state_manager
 * @{
 *
 * @file
 * @brief       Token State Manager implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */
#include "state_manager.h"
#include "uwb_epoch.h"

#include "nanocbor/nanocbor.h"
#include "fmt.h"
#include "luid.h"

#include "board.h"
#include "periph/gpio.h"

#if (IS_USED(MODULE_COAP_UTILS))
#include "coap/utils.h"
#include "net/coap.h"
#endif

#if (IS_USED(MODULE_SECURITY_CTX))
#include "ztimer.h"
#include "event/periodic.h"
#include "event/callback.h"

#include "security_ctx.h"
#include "edhoc/coap.h"

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
#include "desire_ble_scan.h"
#endif
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_DEBUG
#endif
#include "log.h"

static bool _infected = false;
static bool _esr = false;
static char _uid[sizeof("DW") + STATE_MANAGER_TOKEN_ID_LEN * 2 + 1];

#if (IS_USED(MODULE_COAP_UTILS))
static sock_udp_ep_t *_remote;
static coap_req_ctx_t _get_ctx;
static coap_block_ctx_t _block_ctx;
/* block message will require a pointer to this uri */
char _ertl_uri[sizeof("/DW") + sizeof("/ertl") + 2 *
               STATE_MANAGER_TOKEN_ID_LEN];
/* forward declaration */
void _esr_callback(int res, void *data, size_t data_len, void *arg);
#endif

#if IS_USED(MODULE_SECURITY_CTX)
static security_ctx_t _sec_ctx;
static edhoc_coap_ctx_t _edhoc_ctx;
static uint8_t ertl_cose_buf[1536];
static uint8_t *ertl_cose_ptr;
static uint8_t ibuf[128];
static uint8_t dbuf[128];

static void _handshake_handler(void *arg)
{
    (void)arg;
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
    if (!desire_ble_is_connected()) {
        return;
    }
#endif
    /* TODO: only try this if connected, handle connection at state manager */
    if (_sec_ctx.valid == false) {
        security_ctx_edhoc_handshake(&_sec_ctx, &_edhoc_ctx.ctx, _remote);
    }
}

static event_periodic_t _handshake_periodic;
static event_callback_t _handshake_event = EVENT_CALLBACK_INIT(
    _handshake_handler, NULL
    );
#endif

void _set_uid(void)
{
#ifndef BOARD_NATIVE
    uint8_t addr[STATE_MANAGER_TOKEN_ID_LEN];
    luid_get(addr, STATE_MANAGER_TOKEN_ID_LEN);
    fmt_bytes_hex(&_uid[(sizeof("DW") - 1)], addr, STATE_MANAGER_TOKEN_ID_LEN);
    _uid[0] = 'D';
    _uid[1] = 'W';
    _uid[sizeof("DW") + 2 * STATE_MANAGER_TOKEN_ID_LEN] = '\0';
#else
    memcpy(_uid, "DW1234", sizeof("DW1234"));
#endif
#if (IS_USED(MODULE_COAP_UTILS))
    sprintf(_ertl_uri, "/%s/ertl", _uid);
#endif
}

void state_manager_init(void)
{
    _infected = false;
    _esr = false;
    _set_uid();
#if (IS_USED(MODULE_COAP_UTILS))
    coap_req_ctx_init(&_get_ctx, _esr_callback, NULL);
    coap_req_ctx_init(&_block_ctx.req_ctx, NULL, NULL);
#endif
}

void state_manager_set_infected_status(bool status)
{
    _infected = status;
    if (_infected) {
        LOG_INFO("[state_manager]: COVID positive!\n");
#ifdef LED1_PIN
        LED1_ON;
    }
    else {
        LED1_OFF;
#endif
    }
}

bool state_manager_get_infected_status(void)
{
    return _infected;
}

size_t state_manager_infected_serialize_cbor(uint8_t *buf, size_t len)
{
    nanocbor_encoder_t enc;

    nanocbor_encoder_init(&enc, buf, len);
    nanocbor_fmt_tag(&enc, STATE_MANAGER_INFECTED_CBOR_TAG);
    nanocbor_fmt_array(&enc, 1);
    nanocbor_fmt_bool(&enc, _infected);
    return nanocbor_encoded_len(&enc);
}

void state_manager_set_esr(bool esr)
{
    LOG_DEBUG("[state_manager]: esr=(%d)\n", esr);
    _esr = esr;
    if (_esr) {
        LOG_INFO("[state_manager]: exposed!\n");
#ifdef LED0_PIN
        LED0_ON;
    }
    else {
        LED0_OFF;
#endif
    }
}

int state_manager_esr_load_cbor(uint8_t *buf, size_t len)
{
    nanocbor_value_t dec;
    nanocbor_value_t arr;
    uint32_t tag;

    nanocbor_decoder_init(&dec, buf, len);
    nanocbor_get_tag(&dec, &tag);
    /* workaround nanocbor bug see https://github.com/bergzand/NanoCBOR/pull/55 */
    if ((uint16_t)tag != STATE_MANAGER_ESR_CBOR_TAG) {
        return -1;
    }
    nanocbor_enter_array(&dec, &arr);
    bool contact;
    if (nanocbor_get_bool(&arr, &contact) == NANOCBOR_OK) {
        state_manager_set_esr(contact);
    }
    else {
        return -1;
    }
    nanocbor_leave_container(&dec, &arr);
    return 0;
}

bool state_manager_get_esr(void)
{
    return _esr;
}

char *state_manager_get_id(void)
{
    return _uid;
}

#if IS_USED(MODULE_COAP_UTILS)
#if IS_USED(MODULE_SECURITY_CTX)
void state_manager_security_init(event_queue_t *queue)
{
    security_ctx_init(&_sec_ctx, (uint8_t *)_uid, strlen(_uid),
                      (uint8_t *)pepper_server_id, sizeof(pepper_server_id));
    /* setup initiator context */
    edhoc_coap_init(&_edhoc_ctx, EDHOC_IS_INITIATOR, (uint8_t*) _uid, strlen(_uid));
    /* set up periodic callback */
    event_periodic_init(&_handshake_periodic, ZTIMER_EPOCH, queue,
                        &_handshake_event.super);
    event_periodic_start(&_handshake_periodic, CONFIG_STATE_MANAGER_EDHOC_S);
}
#endif

void _esr_callback(int res, void *data, size_t data_len, void *arg)
{
    (void)arg;
    if (res != 0 || (data_len == 0 || data == NULL)) {
        LOG_DEBUG("[state manager]: esr get failed=(%d)\n", res);
        return;
    }
#if IS_USED(MODULE_SECURITY_CTX)
    if (_sec_ctx.valid) {
        uint8_t out[128];
        size_t out_len = 0;
        LOG_DEBUG("[state_manager]: attempt to decode...");
        int ret = security_ctx_decode(
            &_sec_ctx, data, data_len, dbuf, sizeof(dbuf), out, &out_len);
        if (ret == 0) {
            LOG_DEBUG("success\n");
            state_manager_esr_load_cbor(out, out_len);
            return;
        }
        /* if failed to decode assume ctx is no longer valid */
        _sec_ctx.valid = false;
        LOG_DEBUG("failed\n");
        return;
    }
#else
    state_manager_esr_load_cbor(data, data_len);
#endif
}

void state_manager_set_remote(sock_udp_ep_t *remote)
{
    _remote = remote;
}

int state_manager_coap_get_esr(void)
{
#if IS_USED(MODULE_SECURITY_CTX)
    if (!_sec_ctx.valid) {
        LOG_DEBUG("[state_manager]: invalid context\n");
        return -1;
    }
#endif

    if (!coap_req_ctx_is_free(&_get_ctx)) {
        LOG_DEBUG("[state_manager]: get context is not freed\n");
        return -1;
    }

    char uri[sizeof("/") + sizeof("/esr") + sizeof("DW") + STATE_MANAGER_TOKEN_ID_LEN * 2];
    sprintf(uri, "/%s/esr", _uid);
    LOG_DEBUG("[state_manager]: fetch esr\n");
    return coap_get(_remote, &_get_ctx, uri, COAP_FORMAT_CBOR, COAP_TYPE_NON);
}

/* TODO: this should handle the worst scenario CBOR length */
static uint8_t ertl_buf[1024];
int state_manager_coap_send_ertl(uwb_epoch_data_t *data)
{
#if IS_USED(MODULE_SECURITY_CTX)
    if (!_sec_ctx.valid) {
        LOG_DEBUG("[state_manager]: invalid context\n");
        return -1;
    }
#endif

    if (!coap_req_ctx_is_free(&_block_ctx.req_ctx)) {
        LOG_DEBUG("[state_manager]: block context is not freed\n");
        return -1;
    }

    size_t data_len = uwb_epoch_serialize_cbor(data, ertl_buf, sizeof(ertl_buf));
    LOG_DEBUG("[state_manager]: serialized ertl, len=(%d)\n", data_len);

    if (data_len > 0) {
#if IS_USED(MODULE_SECURITY_CTX)
        ertl_cose_ptr = NULL;
        size_t cose_len = security_ctx_encode(&_sec_ctx,
                                              ertl_buf, data_len,
                                              ertl_cose_buf, sizeof(ertl_cose_buf),
                                              &ertl_cose_ptr);
        LOG_DEBUG("[state_manager]: encrypted ertl, len=(%d)\n", cose_len);
        return coap_block_post(_remote, &_block_ctx, ertl_cose_ptr, cose_len,
                        _ertl_uri, COAP_FORMAT_CBOR, COAP_TYPE_NON);
#else
        return coap_block_post(_remote, &_block_ctx, ertl_buf, data_len,
                        _ertl_uri, COAP_FORMAT_CBOR, COAP_TYPE_NON);
#endif
    }
    return -1;
}

int state_manager_coap_send_infected(void)
{
#if IS_USED(MODULE_SECURITY_CTX)
    if (!_sec_ctx.valid) {
        LOG_DEBUG("[state_manager]: invalid context\n");
        return -1;
    }
#endif
    uint8_t buf[8];
    char uri[sizeof("/DW") + sizeof("/infected") + 2 *
             STATE_MANAGER_TOKEN_ID_LEN];

    sprintf(uri, "/%s/infected", _uid);
    size_t len = state_manager_infected_serialize_cbor(buf, sizeof(buf));
    LOG_DEBUG("[state_manager]: send infected=%d, len=(%d)\n",
              state_manager_get_infected_status(), len);
#if IS_USED(MODULE_SECURITY_CTX)
    uint8_t *out = NULL;
    size_t out_len = security_ctx_encode(&_sec_ctx,
                                         buf, len,
                                         ibuf, sizeof(ibuf),
                                         &out);
    LOG_DEBUG("[security_manager]: encoded infected len=(%d)\n", out_len);
    return coap_post(_remote, NULL, out, out_len, uri, COAP_FORMAT_CBOR, COAP_TYPE_CON);
#else
    return coap_post(_remote, NULL, buf, len, uri, COAP_FORMAT_CBOR, COAP_TYPE_CON);
#endif
}
#endif
