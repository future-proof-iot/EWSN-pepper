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

#ifndef PEPPER_SERVER_UTILS_H
#define PEPPER_SERVER_UTILS_H

#include "epoch.h"
#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   DW%/infected resource CBOR Tag
 */
#ifndef CONFIG_PEPPER_SRV_INFECTED_CBOR_TAG
#define CONFIG_PEPPER_SRV_INFECTED_CBOR_TAG     (0xCAFA)
#endif

/**
 * @brief   DW%/esr resource CBOR Tag
 */
#ifndef CONFIG_PEPPER_SRV_ESR_CBOR_TAG
#define CONFIG_PEPPER_SRV_ESR_CBOR_TAG          (0xCAFF)
#endif

/**
 * @brief   Serialize infection status into CBOR message
 *
 * @param[inout]    buf     buffer for serialization
 * @param[in]       len     size of the input buffer
 * @param[in]       infected    infections status value
 *
 * @return  encoded length
 */
size_t pepper_srv_infected_serialize_cbor(uint8_t *buf, size_t len, bool infected);

/**
 * @brief   Serialize exposure status into CBOR message
 *
 * @param[inout]    buf     buffer for serialization
 * @param[in]       len     size of the input buffer
 * @param[in]       esr     exposure status value
 *
 * @return  encoded length
 */
int pepper_srv_esr_serialize_cbor(uint8_t *buf, size_t len, bool esr);

/**
 * @brief   Loads exposure status value from CBOR buffer
 *
 * @param[in]       buf     buffer containing CBOR msg
 * @param[in]       len     msg size
 * @param[inout]    esr     exposure status value
 *
 * @return  0 if successfully loaded, <0 otherwise
 */
int pepper_srv_esr_load_cbor(uint8_t *buf, size_t len, bool *esr);

/**
 * @brief   Loads infection value from CBOR buffer
 *
 * @param[in]       buf         buffer containing CBOR msg
 * @param[in]       len         msg size
 * @param[inout]    infected    infections status value
 *
 * @return  0 if successfully loaded, <0 otherwise
 */
int pepper_srv_infected_load_cbor(uint8_t *buf, size_t len, bool *infected);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_SERVER_UTILS_H */
/** @} */
