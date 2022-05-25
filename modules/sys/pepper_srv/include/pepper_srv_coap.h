/*
 * Copyright (C) 2022 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_pepper_srv
 *
 * @brief       Serialization utilities
 *
 * @{
 *
 * @file
 *
 * @author      Roudy Dagher <roudy.dagher@inria.fr>
 */

#ifndef PEPPER_SERVER_COAP_H
#define PEPPER_SERVER_COAP_H

#include "epoch.h"
#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_PEPPER_SRV_COAP_HOST
#define CONFIG_PEPPER_SRV_COAP_HOST         "fd00:dead:beef::1"
#endif

#ifndef CONFIG_PEPPER_SRV_COAP_PORT
#define CONFIG_PEPPER_SRV_COAP_PORT         5683
#endif

void pepper_srv_coap_init_remote(char *addr_str, uint16_t port);

/**
 * @brief   PEPPER server CTX id 'PEPPER'
 */
static const uint8_t pepper_server_id[] =
{
    0x50, 0x45, 0x50, 0x50, 0x45, 0x52
};

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_SERVER_COAP_H */
/** @} */
