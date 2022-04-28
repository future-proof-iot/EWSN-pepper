/*
 * Copyright (C) 2022 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_pepper_srv
 * @{
 *
 * @file
 * @brief       Pepper Server Interface implementation
 *
 * @author      Roudy Dagher <roudy.dagher@inria.fr>
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include "pepper.h"
#include "pepper_srv.h"
#include "pepper_srv_utils.h"
#include "pepper_srv_coap.h"
#include "coap/utils.h"

#include "xfa.h"
#include "mutex.h"
#include "event/callback.h"
#include "event/timeout.h"

#include "security_ctx.h"
#include "edhoc/coap.h"

#ifndef CONFIG_COAPS_ERTL_PAYLOAD_BUFFER
#define CONFIG_COAPS_ERTL_PAYLOAD_BUFFER            1024
#endif

#ifndef CONFIG_COAPS_ERTL_COSE_BUFFER
#define CONFIG_COAPS_ERTL_COSE_BUFFER               1546
#endif

#ifndef CONFIG_COAPS_INF_COSE_BUFFER
#define CONFIG_COAPS_INF_COSE_BUFFER                128
#endif

#ifndef CONFIG_COAPS_ESR_COSE_BUFFER
#define CONFIG_COAPS_ESR_COSE_BUFFER                128
#endif

/**
 * @brief   Frequency in seconds at which attemp EDHOC keys exchanges
 */
#ifndef CONFIG_COAPS_HANDSHAKE_RETRY_WAIT
#define CONFIG_COAPS_HANDSHAKE_RETRY_WAIT           (30 * MS_PER_SEC)
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_DEBUG
#endif
#include "log.h"

XFA_USE_CONST(pepper_srv_endpoint_t, pepper_srv_endpoints);

static struct {
    bool exposed;
    bool infected;
} _coap_state = { false, false};

static event_queue_t *_evt_queue;

/* TODO: these could be moved to common module */
static sock_udp_ep_t _remote;
static coap_req_ctx_t _get_ctx;
static coap_block_ctx_t _block_ctx;
static char _ertl_uri[sizeof("/DW") + sizeof("/ertl") + 2 * PEPPER_UID_LEN];
static char _inf_uri[sizeof("/DW") + sizeof("/infected") + 2 * PEPPER_UID_LEN];
static char _esr_uri[sizeof("/DW") + sizeof("/esr") + 2 * PEPPER_UID_LEN];
static mutex_t _wait_esr = MUTEX_INIT;

static security_ctx_t _sec_ctx;
static edhoc_coap_ctx_t _edhoc_ctx;
/* pointer to the encrypted cose object */
/* TODO: this should handle the worst scenario CBOR length */
static uint8_t _ertl_buf[CONFIG_COAPS_ERTL_PAYLOAD_BUFFER];
static uint8_t _ertl_cose_buf[CONFIG_COAPS_ERTL_COSE_BUFFER];
static uint8_t _esr_cose_buf[CONFIG_COAPS_ESR_COSE_BUFFER];
static uint8_t _inf_cose_buf[CONFIG_COAPS_INF_COSE_BUFFER];

/* instead of periodic handshake we will try a handshake whenever we need
   to send, and if we fail block retries for a while */
static void _handshake_reset(void *arg);
static bool _handshake_retry = true;
static event_timeout_t _handshake_timeout;
static event_callback_t _handshake_event = EVENT_CALLBACK_INIT(
    _handshake_reset, NULL
    );

static void _handshake_reset(void *arg)
{
    (void)arg;
    _handshake_retry = true;
}

static bool _check_security_ctx(void)
{
    if (_sec_ctx.valid == false && _handshake_retry) {
        security_ctx_edhoc_handshake(&_sec_ctx, &_edhoc_ctx.ctx, &_remote);
        _handshake_retry = false;
        event_timeout_set(&_handshake_timeout, CONFIG_COAPS_HANDSHAKE_RETRY_WAIT);
    }
    return _sec_ctx.valid;
}

void _esr_callback(int res, void *data, size_t data_len, void *arg)
{
    (void)arg;
    if (res != 0 || (data_len == 0 || data == NULL)) {
        LOG_DEBUG("[pepper_srv] coaps: esr get failed=(%d)\n", res);
        mutex_unlock(&_wait_esr);
        goto exit;
    }
    if (_check_security_ctx()) {
        uint8_t out[32];
        size_t out_len = 0;
        LOG_INFO("[pepper_srv] coaps: decrypt esr... ");
        int ret = security_ctx_decode(
            &_sec_ctx, data, data_len, _esr_cose_buf, sizeof(_esr_cose_buf), out, &out_len);
        if (ret == 0) {
            LOG_INFO("success\n");
            pepper_srv_esr_load_cbor(out, out_len, &_coap_state.exposed);
            goto exit;
        }
        /* if failed to decode assume ctx is no longer valid */
        _sec_ctx.valid = false;
        LOG_INFO("failed\n");
        goto exit;
    }

    exit:
        mutex_unlock(&_wait_esr);
}

int _coaps_srv_init(event_queue_t *evt_queue)
{
    _evt_queue = evt_queue;
    sprintf(_ertl_uri, "/%s/ertl", pepper_get_uid_str());
    sprintf(_inf_uri, "/%s/infected", pepper_get_uid_str());
    sprintf(_esr_uri, "/%s/esr", pepper_get_uid_str());

    coap_init_remote(&_remote, CONFIG_PEPPER_SRV_COAP_HOST, CONFIG_PEPPER_SRV_COAP_PORT);

    coap_req_ctx_init(&_get_ctx, _esr_callback, NULL);
    coap_req_ctx_init(&_block_ctx.req_ctx, NULL, NULL);

    security_ctx_init(&_sec_ctx, (uint8_t *)pepper_get_uid_str(), strlen(pepper_get_uid_str()),
                      (uint8_t *)pepper_server_id, sizeof(pepper_server_id));

    /* setup initiator context */
    if (edhoc_coap_init(&_edhoc_ctx, EDHOC_IS_INITIATOR, (uint8_t *)pepper_get_uid_str(),
                        strlen(pepper_get_uid_str())) == 0) {
        event_timeout_ztimer_init(&_handshake_timeout, ZTIMER_MSEC, _evt_queue,
                                  &_handshake_event.super);
        _handshake_retry = true;
    }
    else {
        LOG_ERROR("[pepper_srv] coaps: failed to initialize security\n");
    }

    return 0;
}

int _coaps_srv_notify_epoch_data(epoch_data_t *epoch_data)
{
    if (!_check_security_ctx()) {
        LOG_DEBUG("[pepper_srv] coaps: invalid context\n");
        return -1;
    }

    if (!mutex_trylock(&_block_ctx.req_ctx.resp_wait)) {
        LOG_DEBUG("[pepper_srv] coaps: block context is not freed\n");
        return -1;
    }
    mutex_unlock(&_block_ctx.req_ctx.resp_wait);

    size_t data_len = contact_data_serialize_all_cbor(epoch_data, _ertl_buf, sizeof(_ertl_buf));

    /* pointer to encrypted object location, since we are blocking on send it can
       remain in function context */
    uint8_t *_ertl_cose_ptr = NULL;
    size_t cose_len = security_ctx_encode(&_sec_ctx,
                                            _ertl_buf, data_len,
                                            _ertl_cose_buf, sizeof(_ertl_cose_buf),
                                            &_ertl_cose_ptr);
    LOG_INFO("[pepper_srv] coaps: encrypted ertl, len=(%d)\n", cose_len);

    if (coap_block_post(&_remote, &_block_ctx, _ertl_cose_ptr, cose_len,
                        _ertl_uri, COAP_FORMAT_CBOR, COAP_TYPE_NON) < 0) {

        LOG_WARNING("[pepper_srv] coaps: ERROR in block post\n");
    }
    /* force a blocking wait */
    mutex_lock(&_block_ctx.req_ctx.resp_wait);
    mutex_unlock(&_block_ctx.req_ctx.resp_wait);
    return 0;
}

int _coaps_srv_notify_infection(bool infected)
{
    if (!_check_security_ctx()) {
        LOG_DEBUG("[pepper_srv] coaps: invalid context\n");
        return -1;
    }

    _coap_state.infected = infected;
    uint8_t buf[8];

    /* serialize */
    size_t len = pepper_srv_infected_serialize_cbor(buf, sizeof(buf), infected);

    LOG_INFO("[pepper_srv] coaps: serialize infected=%s, len=(%d)\n",
             infected ? "true": "false", len);

    /* encrypt */
    uint8_t *out = NULL;
    size_t out_len = security_ctx_encode(&_sec_ctx,
                                         buf, len,
                                         _inf_cose_buf, sizeof(_inf_cose_buf),
                                         &out);
    LOG_INFO("[pepper_srv] coaps: encrypted infected status, len=(%d)\n", out_len);

    return coap_post(&_remote, NULL, out, out_len, _inf_uri, COAP_FORMAT_CBOR, COAP_TYPE_CON);
}

int _coaps_srv_request_exposure(bool *esr /*out*/)
{
    *esr = _coap_state.exposed;

    if (!_check_security_ctx()) {
        LOG_DEBUG("[pepper_srv] coaps: invalid context\n");
        return -1;
    }

    if (!coap_req_ctx_is_free(&_get_ctx)) {
        LOG_DEBUG("[pepper_srv] coaps: get context is not freed\n");
        return -1;
    }

    LOG_INFO("[pepper_srv] coaps: fetch esr at %s\n", _esr_uri);

    /* non blocking get */
    if (coap_get(&_remote, &_get_ctx, _esr_uri, COAP_FORMAT_CBOR, COAP_TYPE_NON) < 0) {
        return -1;
    }

    /* TODO: rexamine this, this is faking the blocking part the signature requires */
    mutex_lock(&_wait_esr);
    return 0;
}

void pepper_srv_coap_init_remote(char *addr_str, uint16_t port)
{
    coap_init_remote(&_remote, addr_str, port);
}

XFA_CONST(pepper_srv_endpoints, 0) pepper_srv_endpoint_t _pepper_srv_coaps = {
    .init = _coaps_srv_init,
    .notify_epoch_data = _coaps_srv_notify_epoch_data,
    .notify_infection = _coaps_srv_notify_infection,
    .request_exposure = _coaps_srv_request_exposure
};
