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

#ifndef UWB_EPOCH_H
#define UWB_EPOCH_H

#include "crypto_manager.h"
#include "uwb_ed.h"
#include "memarray.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UWB_EPOCH CBOR tag
 */
#define UWB_EPOCH_CBOR_TAG      (0xCAFE)

/**
 * @brief   Step between windows in seconds
 */
#ifndef CONFIF_EPOCH_MAX_ENOCUNTERS
#define CONFIG_EPOCH_MAX_ENCOUNTERS        8
#endif

/**
 * @brief   Size of the uwb epoch data memory buffer
 */
#ifndef CONFIG_UWB_EPOCH_DATA_BUF_SIZE
#define CONFIG_UWB_EPOCH_DATA_BUF_SIZE      2
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
    uint32_t timestamp;                                         /**< epoch timestamp seconds*/
    uwb_contact_data_t contacts[CONFIG_EPOCH_MAX_ENCOUNTERS];   /**< possible contacts */
    crypto_manager_keys_t *keys;                                /**< keys */
} uwb_epoch_data_t;

/**
 * @brief   UWB Epoch data memory manager structure
 */
typedef struct uwb_epoch_data_memory_manager {
    uint8_t buf[CONFIG_UWB_EPOCH_DATA_BUF_SIZE * sizeof(uwb_epoch_data_t)]; /**< Task buffer */
    memarray_t mem;                                                         /**< Memarray management */
} uwb_epoch_data_memory_manager_t;

/**
 * @brief   Start of an uwb_epoch
 *
 * @param[inout]    uwb_epoch   the uwb_epoch data element to initialize
 * @param[in]       timestamp   a timestamp in seconds relative to the service
 *                              start time
 * @param[in]       keys        the crypto keys for this uwb_epoch, can be NULL
 */
void uwb_epoch_init(uwb_epoch_data_t *uwb_epoch, uint32_t timestamp,
                    crypto_manager_keys_t *keys);
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

/**
 * @brief   Returns the amount of contacts in an epoch
 *
 * @param[in]       uwb_epoch       the uwb_epoch data
 */
uint8_t uwb_epoch_contacts(uwb_epoch_data_t *epoch);

/**
 * @brief   Serialize (CBOR) uwb_epoch data
 *
 * @param[in]       uwb_epoch       the uwb_epoch data to serialize
 * @param[in]       buf             pointer to allocated encoding buffer
 * @param[in]       len             length of encoding buffer
 *
 * @return  Encoded length
 */
size_t uwb_epoch_serialize_cbor(uwb_epoch_data_t *epoch, uint8_t *buf,
                                size_t len);

/**
 * @brief   Loads serialized uwb_epoch data (no keys)
 *
 * @param[in]       buf             pointer to encoded data
 * @param[in]       len             size of the encoded data
 * @param[in]       uwb_epoch       the uwb_epoch data to deserialize
 *
 * @return  0 on success, <0 otherwise
 */
int uwb_epoch_load_cbor(uint8_t *buf, size_t len, uwb_epoch_data_t *epoch);

/**
 * @brief   Serialize (CBOR) uwb_contact_data_t
 *
 * @param[in]       contact         the uwb_epoch data to serialize
 * @param[in]       buf             pointer to allocated encoding buffer
 * @param[in]       len             length of encoding buffer
 *
 * @return  Encoded length
 */
size_t uwb_contact_serialize_cbor(uwb_contact_data_t *contact,
                                  uint32_t timestamp,
                                  uint8_t *buf, size_t len);

/**
 * @brief   Loads serialized uwb_contact data
 *
 * @param[in]       buf             pointer to encoded data
 * @param[in]       len             size of the encoded data
 * @param[in]       contact         the uwb_contact_data data to deserialize
 * @param[in]       timestamp       the timestamp for that contact
 *
 * @return  0 on success, <0 otherwise
 */
int uwb_contact_load_cbor(uint8_t *buf, size_t len, uwb_contact_data_t *contact,
                          uint32_t *timestamp);

/**
 * @brief   Init the memory manager
 *
 * @param[in]   manager     the memory manager structure to init
 */
void uwb_epoch_data_memory_manager_init(
    uwb_epoch_data_memory_manager_t *manager);

/**
 * @brief   Free an allocated element
 *
 * @param[in]       manager         the memory manager
 * @param[inout]    uwb_epoch_data  the encounter data to to free
 */
void uwb_epoch_data_memory_manager_free(
    uwb_epoch_data_memory_manager_t *manager,
    uwb_epoch_data_t *uwb_epoch_data);

/**
 * @brief   Allocate some space for a new encounter data entry
 *
 * @param[in]       manager         the memory manager
 *
 * @returns         pointer to the allocated encounter data structure
 */
uwb_epoch_data_t *uwb_epoch_data_memory_manager_calloc(
    uwb_epoch_data_memory_manager_t *manager);


#ifdef __cplusplus
}
#endif

#endif /* UWB_EPOCH_H */
/** @} */
