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

#ifndef CONFIG_COAP_ERTL_PAYLOAD_BUFFER
#define CONFIG_COAP_ERTL_PATLOAD_BUFFER    1024
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

static sock_udp_ep_t _remote;
static coap_req_ctx_t _get_ctx;
static coap_block_ctx_t _block_ctx;
static char _ertl_uri[sizeof("/DW") + sizeof("/ertl") + 2 * PEPPER_UID_LEN];
static char _inf_uri[sizeof("/DW") + sizeof("/infected") + 2 * PEPPER_UID_LEN];
static char _esr_uri[sizeof("/DW") + sizeof("/esr") + 2 * PEPPER_UID_LEN];
static mutex_t _wait_esr = MUTEX_INIT;

/* TODO: this should handle the worst scenario CBOR length */
static uint8_t _ertl_buf[CONFIG_COAP_ERTL_PATLOAD_BUFFER];

void _esr_callback(int res, void *data, size_t data_len, void *arg)
{
    (void)arg;
    if (res != 0 || (data_len == 0 || data == NULL)) {
        LOG_DEBUG("[pepper_srv] coap: esr get failed=(%d)\n", res);
        mutex_unlock(&_wait_esr);
        return;
    }
    pepper_srv_esr_load_cbor(data, data_len, &_coap_state.exposed);
    mutex_unlock(&_wait_esr);
}

int _coap_srv_init(event_queue_t *evt_queue)
{
    _evt_queue = evt_queue;
    sprintf(_ertl_uri, "/%s/ertl", pepper_get_uid_str());
    sprintf(_inf_uri, "/%s/infected", pepper_get_uid_str());
    sprintf(_esr_uri, "/%s/esr", pepper_get_uid_str());

    coap_init_remote(&_remote, CONFIG_PEPPER_SRV_COAP_HOST, CONFIG_PEPPER_SRV_COAP_PORT);

    coap_req_ctx_init(&_get_ctx, _esr_callback, NULL);
    coap_req_ctx_init(&_block_ctx.req_ctx, NULL, NULL);
    return 0;
}

int _coap_srv_notify_epoch_data(epoch_data_t *epoch_data)
{
    if (!mutex_trylock(&_block_ctx.req_ctx.resp_wait)) {
        LOG_DEBUG("[pepper_srv] coap: block context is not freed\n");
        return -1;
    }
    mutex_unlock(&_block_ctx.req_ctx.resp_wait);

    size_t data_len = contact_data_serialize_all_cbor(epoch_data, _ertl_buf, sizeof(_ertl_buf));

    LOG_INFO("[pepper_srv] coap: send ertl to %s\n", _ertl_uri);
    if (coap_block_post(&_remote, &_block_ctx, _ertl_buf, data_len,
                        _ertl_uri, COAP_FORMAT_CBOR, COAP_TYPE_NON) < 0) {

        LOG_WARNING("[pepper_srv] coap: ERROR in block post\n");
    }
    /* force a blocking wait */
    mutex_lock(&_block_ctx.req_ctx.resp_wait);
    mutex_unlock(&_block_ctx.req_ctx.resp_wait);
    return 0;
}

int _coap_srv_notify_infection(bool infected)
{
    _coap_state.infected = infected;
    uint8_t buf[8];

    size_t len = pepper_srv_infected_serialize_cbor(buf, sizeof(buf), infected);

    LOG_INFO("[pepper_srv] coap : serialize infected=%s, len=(%d)\n",
             infected ? "true": "false", len);

    /* non blocking post */
    return coap_post(&_remote, NULL, buf, len, _inf_uri, COAP_FORMAT_CBOR, COAP_TYPE_CON);
}

int _coap_srv_request_exposure(bool *esr /*out*/)
{
    *esr = _coap_state.exposed;

    if (!coap_req_ctx_is_free(&_get_ctx)) {
        LOG_DEBUG("[pepper_srv] coap: get context is not freed\n");
        return -1;
    }

    LOG_INFO("[pepper_srv] coap: fetch esr at %s\n", _esr_uri);

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

XFA_CONST(pepper_srv_endpoints, 0) pepper_srv_endpoint_t _pepper_srv_coap = {
    .init = _coap_srv_init,
    .notify_epoch_data = _coap_srv_notify_epoch_data,
    .notify_infection = _coap_srv_notify_infection,
    .request_exposure = _coap_srv_request_exposure
};
