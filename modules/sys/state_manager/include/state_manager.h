/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_state_manager Token State Manager
 * @ingroup     sys
 * @brief       Module handling the Token global state
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <inttypes.h>
#include <stdbool.h>

#include "kernel_defines.h"
#if (IS_USED(MODULE_COAP_UTILS))
#include "net/sock/udp.h"
#endif
#include "uwb_epoch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Frequency in seconds at which attemp EDHOC keys exchanges
 */
#ifndef CONFIG_STATE_MANAGER_EDHOC_S
#define CONFIG_STATE_MANAGER_EDHOC_S      5
#endif

#if IS_USED(MODULE_SECURITY_CTX)
/**
 * @brief   PEPPER server CTX id 'PEPPER'
 */
static const uint8_t pepper_server_id[] =
{
    0x50, 0x45, 0x50, 0x50, 0x45, 0x52
};
#endif

/**
 * @brief   DW%/infected resource CBOR Tag
 */
#define STATE_MANAGER_INFECTED_CBOR_TAG     (0xCAFA)

/**
 * @brief   DW%/esr resource CBOR Tag
 */
#define STATE_MANAGER_ESR_CBOR_TAG          (0xCAFF)

/**
 * @brief   Token ID length in bytes
 */
#define STATE_MANAGER_TOKEN_ID_LEN          (2)

/**
 *
 * @brief   Initiate state manager
 */
void state_manager_init(void);

/**
 * @brief   Return infection status
 *
 * @return  true if infected, false otherwise
 */
bool state_manager_get_infected_status(void);

/**
 * @brief   Set infection status
 *
 * @param[in]       status  infection status
 */
void state_manager_set_infected_status(bool status);

/**
 * @brief   Serialize infection status into CBOR message
 *
 * @param[inout]    buf     buffer for serialization
 * @param[in]       len     size of the input buffer
 *
 * @return  encoded length
 */
size_t state_manager_infected_serialize_cbor(uint8_t *buf, size_t len);

/**
 * @brief   Sets the exposure status
 *
 * @param[in]       esr     exposure status
 */
void state_manager_set_esr(bool esr);

/**
 * @brief   Returns the exposure status
 *
 * @return  true if exposed, false otherwise
 */
bool state_manager_get_esr(void);

/**
 * @brief   Loads exposure status value from CBOR buffer
 *
 * @param[in]       buf     buffer containing CBOR msg
 * @param[in]       len     msg size
 *
 * @return  0 if successfully loaded, <0 otherwise
 */
int state_manager_esr_load_cbor(uint8_t *buf, size_t len);

/**
 * @brief   Return the Token id
 *
 * @return  pointer to the string id
 */
char *state_manager_get_id(void);

#if (IS_USED(MODULE_COAP_UTILS))
/**
 * @brief   Set the remote endpoint
 *
 * @param[in]       remote  the remote endpoint
 */
void state_manager_set_remote(sock_udp_ep_t *remote);

/**
 * @brief   GET the exposure status from the remote endpoint
 *
 */
int state_manager_coap_get_esr(void);

/**
 * @brief   POST the epoch data to the remote endpoint
 *
 * @param[in]       data    the epoch_data to serialize and POST
 */
int state_manager_coap_send_ertl(uwb_epoch_data_t *data);

/**
 * @brief   POST the infection status to the remote endpoint
 *
 */
int state_manager_coap_send_infected(void);
#endif

#if IS_USED(MODULE_SECURITY_CTX)
/**
 * @brief   Sets up a periodic task to attemp EDHOC key exchange if
 *          IPV6 connectivity is found
 */
void state_manager_security_init(event_queue_t* queue);
#endif

#ifdef __cplusplus
}
#endif

#endif /* STATE_MANAGER_H */
/** @} */
