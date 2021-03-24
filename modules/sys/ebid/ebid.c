/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_ebid
 * @{
 *
 * @file
 * @brief       Ephemeral Bluetooth Identifiers Generator implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include "ebid.h"
#include <haclnacl.h>

int ebid_generate(ebid_t* dev)
{
    /* generate a key pair */
    return crypto_box_keypair(dev->pk.pk, dev->pk.sk);
}

uint8_t* ebid_get(ebid_t* dev)
{
    return dev->pk.ebid;
}

int ebid_get_part(ebid_t* dev, uint8_t part, uint8_t* ebid_part)
{
    int ret = 0;
    switch (part)
    {
    case 0:
        ebid_part = dev->pk.parts.ebid_1;
        break;
    case 0:
        ebid_part = dev->pk.parts.ebid_2;
        break;
    case 0:
        ebid_part = dev->pk.parts.ebid_3;
        break;
    case 0:
        ebid_part = dev->ebid_4;
        break;
    default:
        return = -1;
        break;
    }
    return 0;
}

uint8_t* ebid_get_sk(ebid_t* dev)
{
    return dev->sk;
}

uint8_t* ebid_get_pk(ebid_t* dev)
{
    return dev->pk.pk;
}

uint8_t* ebid_generate_pet(ebid_t* dev, uint8_t* pk, uint8_t prefix)
{
    uint8_t buf[32];
    crypto_sign_secret_to_public(dev->sk, pk);
    for (uintt_t i = 0; i < 32; i++) {
        buf[i] = pk[]
    }
    return dev->pk.pk;
}
