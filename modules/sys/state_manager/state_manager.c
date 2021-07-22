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

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_DEBUG
#endif
#include "log.h"

static bool _infected = false;
static bool _esr = false;
static char _uid[STATE_MANAGER_TOKEN_ID_LEN * 2 + 1];

#if (IS_USED(MODULE_COAP_UTILS))
static coap_get_ctx_t _get_ctx;
static coap_block_ctx_t _block_ctx;
/* block message will require a pointer to this uri */
char _ertl_uri[sizeof("/DW") + sizeof("/ertl") + 2 *
               STATE_MANAGER_TOKEN_ID_LEN];
#endif

void _set_uid(void)
{
    uint8_t addr[STATE_MANAGER_TOKEN_ID_LEN];

    luid_get(addr, STATE_MANAGER_TOKEN_ID_LEN);
    fmt_bytes_hex(_uid, addr, STATE_MANAGER_TOKEN_ID_LEN);
    _uid[2 * STATE_MANAGER_TOKEN_ID_LEN] = '\0';
#if (IS_USED(MODULE_COAP_UTILS))
    sprintf(_ertl_uri, "/DW%s/ertl", _uid);
#endif
}

void state_manager_init(void)
{
    _infected = false;
    _esr = false;
    _set_uid();
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

#if (IS_USED(MODULE_COAP_UTILS))
void _esr_callback(uint8_t *data, uint16_t len, void *arg)
{
    (void)arg;
    assert(data && len > 0);
    state_manager_esr_load_cbor(data, len);
}

void state_manager_coap_get_esr(sock_udp_ep_t *remote)
{
    char uri[sizeof("/DW") + sizeof("/esr") +  2 * STATE_MANAGER_TOKEN_ID_LEN];

    sprintf(uri, "/DW%s/esr", _uid);
    LOG_DEBUG("[state_manager]: fetch esr\n");
    coap_get_ctx_init(&_get_ctx, _esr_callback, NULL);
    coap_get(remote, &_get_ctx, uri, COAP_FORMAT_CBOR, COAP_TYPE_NON);
}

void state_manager_coap_send_ertl(sock_udp_ep_t *remote, uwb_epoch_data_t *data,
                                  uint8_t *buf, size_t len)
{
    size_t data_len = uwb_epoch_serialize_cbor(data, buf, len);

    LOG_DEBUG("[state_manager]: send ertl, len=(%d)\n", data_len);

    if (data_len > 0) {
        coap_block_ctx_init(&_block_ctx, buf, data_len, _ertl_uri, NULL, NULL,
                            COAP_FORMAT_CBOR);
        coap_block_post(remote, &_block_ctx);
    }
}

void state_manager_coap_send_infected(sock_udp_ep_t *remote)
{
    uint8_t buf[8];
    char uri[sizeof("/DW") + sizeof("/infected") + 2 *
             STATE_MANAGER_TOKEN_ID_LEN];

    sprintf(uri, "/DW%s/infected", _uid);
    size_t len = state_manager_infected_serialize_cbor(buf, sizeof(buf));
    LOG_DEBUG("[state_manager]: send infected=%d\n",
              state_manager_get_infected_status());
    coap_post(remote, NULL, uri, buf, len, COAP_FORMAT_CBOR, COAP_TYPE_CON);
}
#endif
