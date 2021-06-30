/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_uwb_epoch UWB Epoch Encounters
 * @ingroup     sys
 * @brief       Desire Encounter Data Structure Per UWB Epoch
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef UBW_EPOCH_H
#define UWB_EPOCH_H

#include "crypto_manager.h"
#include "uwb_ed.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Step between windows in seconds
 */
#ifndef CONFIF_EPOCH_MAX_ENOCUNTERS
#define CONFIG_EPOCH_MAX_ENCOUNTERS        8
#endif

/**
 * @brief   Contact data
 */
typedef struct uwb_contact_data {
    pet_t pet;                              /**< pet pair */
    uint16_t req_count;                     /**< request count */
    uint16_t exposure_s;                    /**< exposure time */
    uint8_t avg_d_cm;                       /**< distance */
} uwb_contact_data_t;

/**
 * @brief   An Epoch Data Structure
 */
typedef struct uwb_epoch_data {
    uint32_t timestamp;                                     /**< epoch timestamp seconds*/
    uwb_contact_data_t contacts[CONFIG_EPOCH_MAX_ENCOUNTERS];   /**< possible contacts */
    crypto_manager_keys_t *keys;                            /**< keys */
} uwb_epoch_data_t;

/**
 * @brief   Start of an uwb_epoch
 *
 * @param[inout]    uwb_epoch       the uwb_epoch data element to initialize
 * @param[in]       timestamp   a timestamp in seconds relative to the service
 *                              start time
 * @param[in]       keys        the crypto keys for this uwb_epoch
 */
void uwb_epoch_init(uwb_epoch_data_t *uwb_epoch, uint16_t timestamp,
                              crypto_manager_keys_t* keys);
/**
 * @brief   To be called at the end of an uwb_epoch to process all encounter data
 *
 * @param[inout]    uwb_epoch       the uwb_epoch data to finalize
 * @param[inout]    list        the data from all encounters during an uwb_epoch,
 *                              the list is cleaned after this function is called
 */
void uwb_epoch_finish(uwb_epoch_data_t *uwb_epoch, uwb_ed_list_t *list);

/**
 * @brief   Serialize (JSON) an print uwb_epoch data
 *
 * @param[in]       uwb_epoch       the uwb_epoch data to serialize
 */
void uwb_epoch_serialize_printf(uwb_epoch_data_t *uwb_epoch);

#ifdef __cplusplus
}
#endif

#endif /* UWB_EPOCH_H */
/** @} */
