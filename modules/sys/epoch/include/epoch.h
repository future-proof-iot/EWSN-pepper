/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_epoch UWB Epoch Encounters
 * @ingroup     sys
 * @brief       Desire Encounter Data Structure Per UWB Epoch
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef EPOCH_H
#define EPOCH_H

#include "crypto_manager.h"
#include "ed.h"
#include "memarray.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EPOCH CBOR tag
 * @{
 */
#define EPOCH_CBOR_TAG                      (0x4544)
#define ED_UWB_CBOR_TAG                     (0x4500)
#define ED_BLE_CBOR_TAG                     (0x4501)
/** @} */

/**
 * @brief   Step between windows in seconds
 */
#ifndef CONFIF_EPOCH_MAX_ENOCUNTERS
#define CONFIG_EPOCH_MAX_ENCOUNTERS                     8
#endif

/**
 * @brief   Size of the uwb epoch data memory buffer
 */
#ifndef CONFIG_EPOCH_DATA_BUF_SIZE
#define CONFIG_EPOCH_DATA_BUF_SIZE                      2
#endif

/**
 * @brief   Set to one to also serialize BLE data when using CBOR
 *
 * @note    This is set to 0 by default since its not supported by the coap-server
 */
#ifndef CONFIG_CONTACT_DATA_SERIALIZE_CBOR_BLE
#define CONFIG_CONTACT_DATA_SERIALIZE_CBOR_BLE          0
#endif

#if IS_USED(MODULE_ED_UWB)
/**
 * @brief   Contact data
 */
typedef struct contact_uwb_data {
    uint16_t req_count;                     /**< request count */
    uint16_t exposure_s;                    /**< exposure time */
    uint16_t avg_d_cm;                      /**< distance */
#if IS_USED(MODULE_ED_UWB_LOS)
    uint16_t avg_los;                       /**< los */
#endif
#if IS_USED(MODULE_ED_UWB_STATS)
    ed_uwb_stats_t stats;
#endif
} contact_uwb_data_t;
#endif

#if IS_USED(MODULE_ED_BLE_WIN)
/**
 * @brief   Contact data
 */
typedef struct contact_ble_win_data {
    rdl_window_t wins[WINDOWS_PER_EPOCH];   /**< individual windows */
    uint16_t exposure_s;                    /**< exposure time */
} contact_ble_win_data_t;
#endif

#if IS_USED(MODULE_ED_BLE)
/**
 * @brief   Contact data
 */
typedef struct contact_ble_data {
    uint16_t scan_count;                    /**< request count */
    uint16_t exposure_s;                    /**< exposure time */
    uint16_t avg_d_cm;                      /**< distance */
    float avg_rssi;                         /**< distance */
} contact_ble_data_t;
#endif

/**
 * @brief   Contact data
 */
typedef struct contact_data {
    pet_t pet;                              /**< pet pair */
#if IS_USED(MODULE_ED_UWB)
    contact_uwb_data_t uwb;                 /**< UWB contact data */
#endif
#if IS_USED(MODULE_ED_BLE_WIN)
    contact_ble_win_data_t ble_win;         /**< Windowed BLE contact data */
#endif
#if IS_USED(MODULE_ED_BLE)
    contact_ble_data_t ble;                 /**< BLE contact data */
#endif
} contact_data_t;

/**
 * @brief   An Epoch Data Structure
 */
typedef struct epoch_data {
    uint32_t timestamp;                                     /**< epoch timestamp seconds*/
    contact_data_t contacts[CONFIG_EPOCH_MAX_ENCOUNTERS];   /**< possible contacts */
    crypto_manager_keys_t *keys;                            /**< keys */
} epoch_data_t;

/**
 * @brief   UWB Epoch data memory manager structure
 */
typedef struct epoch_data_memory_manager {
    uint8_t buf[CONFIG_EPOCH_DATA_BUF_SIZE * sizeof(epoch_data_t)]; /**< Task buffer */
    memarray_t mem;                                                 /**< Memarray management */
} epoch_data_memory_manager_t;

/**
 * @brief   Start of an epoch
 *
 * @param[inout]    epoch   the epoch data element to initialize
 * @param[in]       timestamp   a timestamp in seconds relative to the service
 *                              start time
 * @param[in]       keys        the crypto keys for this epoch, can be NULL
 */
void epoch_init(epoch_data_t *epoch, uint32_t timestamp, crypto_manager_keys_t *keys);
/**
 * @brief   To be called at the end of an epoch to process all encounter data
 *
 * @param[inout]    epoch       the epoch data to finalize
 * @param[inout]    list        the data from all encounters during an epoch,
 *                              the list is cleaned after this function is called
 */
void epoch_finish(epoch_data_t *epoch, ed_list_t *list);

/**
 * @brief   Returns the amount of contacts in an epoch
 *
 * @param[in]       epoch       the epoch data
 */
uint8_t epoch_contacts(epoch_data_t *epoch);

/**
 * @brief   Serialize (CBOR) contact_uwb_data_t
 *
 * @param[in]       contact         the epoch data to serialize
 * @param[in]       timestamp       the timestamp for that contact
 * @param[in]       buf             pointer to allocated encoding buffer
 * @param[in]       len             length of encoding buffer
 *
 * @return  Encoded length
 */
size_t contact_data_serialize_cbor(contact_data_t *contact, uint32_t timestamp,
                                   uint8_t *buf, size_t len);
/**
 * @brief   Loads serialized contact_uwb data
 *
 * @param[in]       buf             pointer to encoded data
 * @param[in]       len             size of the encoded data
 * @param[in]       contact         the contact_uwb_data data to deserialize
 * @param[in]       timestamp       the timestamp for that contact
 *
 * @return  0 on success, <0 otherwise
 */
int contact_data_load_cbor(uint8_t *buf, size_t len, contact_data_t *contact,
                           uint32_t *timestamp);

/**
 * @brief   Serialize (CBOR) epoch data
 *
 * @param[in]       epoch           the epoch data to serialize
 * @param[in]       buf             pointer to allocated encoding buffer
 * @param[in]       len             length of encoding buffer
 *
 * @return  Encoded length
 */
size_t contact_data_serialize_all_cbor(epoch_data_t *epoch, uint8_t *buf, size_t len);


/**
 * @brief   Serialize (JSON) epoch data
 *
 * @param[in]       epoch           the epoch data to serialize
 * @param[in]       buf             pointer to allocated encoding buffer
 * @param[in]       len             length of encoding buffer
 *
 * @return  Encoded length
 */
size_t contact_data_serialize_all_json(epoch_data_t *epoch, uint8_t *buf,
                                       size_t len, const char *prefix);

/**
 * @brief   Loads serialized epoch data (no keys)
 *
 * @param[in]       buf             pointer to encoded data
 * @param[in]       len             size of the encoded data
 * @param[in]       epoch           the epoch data to deserialize
 *
 * @return  0 on success, <0 otherwise
 */
int contact_data_load_all_cbor(uint8_t *buf, size_t len, epoch_data_t *epoch);

/**
 * @brief   Serialize (JSON) an print epoch data
 *
 * @param[in]       epoch       the epoch data to serialize
 */
void contact_data_serialize_all_printf(epoch_data_t *epoch, const char *prefix);

/**
 * @brief   Init the memory manager
 *
 * @param[in]   manager     the memory manager structure to init
 */
void epoch_data_memory_manager_init(epoch_data_memory_manager_t *manager);

/**
 * @brief   Free an allocated element
 *
 * @param[in]       manager         the memory manager
 * @param[inout]    epoch_data  the encounter data to to free
 */
void epoch_data_memory_manager_free(epoch_data_memory_manager_t *manager,
                                    epoch_data_t *epoch_data);

/**
 * @brief   Allocate some space for a new encounter data entry
 *
 * @param[in]       manager         the memory manager
 *
 * @returns         pointer to the allocated encounter data structure
 */
epoch_data_t *epoch_data_memory_manager_calloc(epoch_data_memory_manager_t *manager);

/**
 * @brief   Populates the contact_data_t structure with random values
 *
 * @param[in]       data       the contact data
 */
void random_contact(contact_data_t *data);

/**
 * @brief   Populates the epoch_data_t structure with random values
 *
 * @param[in]       epoch       the epoch data
 */
void random_epoch(epoch_data_t *data);

bool epoch_valid_contact(contact_data_t *data);

typedef enum {
    EPOCH_SERIALIZE_CBOR,
    EPOCH_SERIALIZE_JSON,
} epoch_serializer_type_t;

typedef enum {
    EPOCH_LABEL_EPOCH = 0,
    EPOCH_LABEL_PETS,
    EPOCH_LABEL_ET,
    EPOCH_LABEL_RT,
    EPOCH_LABEL_UWB,
    EPOCH_LABEL_BLE,
    EPOCH_LABEL_COUNT,
    EPOCH_LABEL_DISTANCE,
    EPOCH_LABEL_RSSI,
    EPOCH_LABEL_EXPOSURE,
} epoch_label_t;

#define EPOCH_LABEL_JSON_EPOCH       "epoch"
#define EPOCH_LABEL_JSON_PETS        "pets"
#define EPOCH_LABEL_JSON_ET          "et"
#define EPOCH_LABEL_JSON_RT          "rt"
#define EPOCH_LABEL_JSON_UWB         "uwb"
#define EPOCH_LABEL_JSON_BLE         "ble"
#define EPOCH_LABEL_JSON_COUNT       "count"
#define EPOCH_LABEL_JSON_DISTANCE    "d_cm"
#define EPOCH_LABEL_JSON_RSSI        "rssi"
#define EPOCH_LABEL_JSON_EXPOSURE    "exp_s"

#ifdef __cplusplus
}
#endif

#endif /* EPOCH_H */
/** @} */
