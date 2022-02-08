/*
 * Copyright (C) 2022 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_pepper_srv Pepper Server Interface
 * @ingroup     sys
 * @brief       Client-side functions for interfacing a pepper server (Desire Server Extension)
 *
 * @{
 *
 * @file
 *
 * @author      Roudy Dagher <roudy.dagher@inria.fr>
 */

#ifndef PEPPER_SERVER_H
#define PEPPER_SERVER_H

#include "epoch.h"
#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Initialize the pepper server interface
 *
 * @param[in]       evt_queue           A shared event queue for firing events
 *
 * @return  a status flag equal 0 if all went fine and a bitmap indicating the plugin endpoint init flags.
 */
int pepper_srv_init(event_queue_t *evt_queue);

/**
 * @brief   Submit new epoch_data_t to the server broker
 *
 * @param[in]       epoch_data           Epoch data to offload to server
 *
 */
void pepper_srv_data_submit(epoch_data_t*);

/**
 * @brief   Notify end of epoch for offloading the encounter data
 *
 * @param[in]       epoch_data           Epoch data to offload to server
 *
 * @return  a status flag equal 0 if all went fine and a bitmap indicating the plugin endpoint init flags.
 */
int pepper_srv_notify_epoch_data(epoch_data_t *epoch_data);

/**
 * @brief   Notify the notification status update
 *
 * @param[in]       infected           Infection boolean flag. True if the node got infected, False otherwise.
 *
 * @return  a status flag equal 0 if all went fine and a bitmap indicating the plugin endpoint init flags.
 */
int pepper_srv_notify_infection(bool infected);

/**
 * @brief   Exposure status requests
 *
 * @param[out]       esr           Exposure status boolean flag. True if the node has been exposed, False otherwise.
 *
 * @return  a status flag equal 0 if all went fine and a bitmap indicating the plugin endpoint init flags.
 */
int pepper_srv_esr(bool *esr);

/**
 * @brief   Endpoint plugin interface for bridging the client with a local/remote server instance.
 */
typedef struct pepper_srv_endpoint {
    int (*init)(event_queue_t *evt_queue);          /**< Shared event queue for posting events required by the plugin endpoint */
    /* core --> endpoint */
    int (*notify_epoch_data)(epoch_data_t *);       /**< Handler to process end of epoch data */
    int (*notify_infection)(bool infected);         /**< Handler to process infection update event */
    /* core <-- endpoint */
    int (*request_exposure)(bool *esr /*out*/);     /**< Handler to query the exposure status */
} pepper_srv_endpoint_t;

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_SERVER_H */
/** @} */
