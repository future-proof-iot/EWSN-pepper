/*
 * Copyright (C) 2022 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <inttypes.h>
#include "pepper_srv_utils.h"
#include "nanocbor/nanocbor.h"

size_t pepper_srv_infected_serialize_cbor(uint8_t *buf, size_t len, bool infected)
{
    nanocbor_encoder_t enc;

    nanocbor_encoder_init(&enc, buf, len);
    nanocbor_fmt_tag(&enc, CONFIG_PEPPER_SRV_INFECTED_CBOR_TAG);
    nanocbor_fmt_array(&enc, 1);
    nanocbor_fmt_bool(&enc, infected);
    return nanocbor_encoded_len(&enc);
}

int pepper_srv_esr_serialize_cbor(uint8_t *buf, size_t len, bool esr)
{
    nanocbor_encoder_t enc;

    nanocbor_encoder_init(&enc, buf, len);
    nanocbor_fmt_tag(&enc, CONFIG_PEPPER_SRV_ESR_CBOR_TAG);
    nanocbor_fmt_array(&enc, 1);
    nanocbor_fmt_bool(&enc, esr);
    return nanocbor_encoded_len(&enc);
}

int pepper_srv_esr_load_cbor(uint8_t *buf, size_t len, bool *esr)
{
    nanocbor_value_t dec;
    nanocbor_value_t arr;
    uint32_t tag;

    nanocbor_decoder_init(&dec, buf, len);
    nanocbor_get_tag(&dec, &tag);
    if (tag != CONFIG_PEPPER_SRV_ESR_CBOR_TAG) {
        return -1;
    }
    nanocbor_enter_array(&dec, &arr);
    bool status;

    if (nanocbor_get_bool(&arr, &status) == NANOCBOR_OK) {
        *esr = status;
    }
    else {
        return -1;
    }
    nanocbor_leave_container(&dec, &arr);
    return 0;
}

int pepper_srv_infected_load_cbor(uint8_t *buf, size_t len, bool *infected)
{
    nanocbor_value_t dec;
    nanocbor_value_t arr;
    uint32_t tag;

    nanocbor_decoder_init(&dec, buf, len);
    nanocbor_get_tag(&dec, &tag);
    if (tag != CONFIG_PEPPER_SRV_INFECTED_CBOR_TAG) {
        return -1;
    }
    nanocbor_enter_array(&dec, &arr);
    bool status;

    if (nanocbor_get_bool(&arr, &status) == NANOCBOR_OK) {
        *infected = status;
    }
    else {
        return -1;
    }
    nanocbor_leave_container(&dec, &arr);
    return 0;
}
