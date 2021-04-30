/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_epoch Epoch Encounters
 * @ingroup     sys
 * @brief       Desire Encounter Data Structure Per Epoch
 *
 * @{
 *
 * TODO:
 *      - use flt16 for the rssi average, maybe libfixmath?
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef EPOCH_H
#define EPOCH_H

#include "crypto_manager.h"
#include "rdl_window.h"
#include "ed.h"

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
typedef struct contact_data {
    uint16_t duration;                      /**< encounter duration */
    pet_t pet;                              /**< pet pair */
    rdl_window_t wins[WINDOWS_PER_EPOCH];   /**< individual windows */
} contact_data_t;

/**
 * @brief   An Epoch Data Structure
 */
typedef struct epoch_data {
    uint16_t timestamp;                                     /**< epoch timestamp */
    contact_data_t contacts[CONFIG_EPOCH_MAX_ENCOUNTERS];   /**< possible contacts */
} epoch_data_t;

/**
 * @brief   Initialize an @ref epoch_data_t structure
 *
 * @param[inout]    epoch       the epoch data element to initialize
 * @param[in]       timestamp   a timestamp in seconds relative to the service
 *                              start time
 */
static inline void epoch_init(epoch_data_t *epoch, uint16_t timestamp)
{
    memset(epoch, '\0', sizeof(epoch_data_t));
    epoch->timestamp = timestamp;
}

/**
 * @brief   To be called at the end of an epoch to process all encounter data
 *
 * @param[inout]    epoch       the epoch data to finalize
 * @param[inout]    list        the data from all encounters during an epoch,
 *                              the list is cleaned after this function is called
 * @param[in]       keys        the crypto keys for this epoch
 */
void epoch_finish(epoch_data_t *epoch, ed_list_t *list,
                  crypto_manager_keys_t *keys);

/**
 * @brief   Serialize (JSON) an print epoch data
 *
 * @param[in]       epoch       the epoch data to serialize
 */
void epoch_serialize_printf(epoch_data_t *epoch);

#ifdef __cplusplus
}
#endif

#endif /* EPOCH_H */
/** @} */
