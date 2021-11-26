/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_ed Encounter Data List (EDL)
 * @ingroup     sys
 * @brief       Desire Encounter Data List
 *
 * Encounter data, rssi or range, is stored as a timped stamped linked list.
 * The linked list is assumed to be accurately ordered. If not it could be
 * ordered by calling @ref clist_sort, but this consists of O(NlogN) runtime
 * cost.
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef UWB_ED_H
#define UWB_ED_H

#include <inttypes.h>
#include <assert.h>
#include <stdbool.h>

#include "kernel_defines.h"
#include "memarray.h"
#include "clist.h"
#include "ebid.h"

#if IS_USED(MODULE_ED_BLE_WIN)
#include "rdl_window.h"
#endif
#include "ed_shared.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Size of the encounter data buffer, this gives an upper limit to
 *          the amount of encounters that can be tracked per epoch
 */
#ifndef CONFIG_ED_BUF_SIZE
#define CONFIG_ED_BUF_SIZE                      (10U)
#endif

/**
 * @brief   Obfuscate the RSSI values before storing
 */
#ifndef CONFIG_ED_BLE_OBFUSCATE_RSSI
#define CONFIG_ED_BLE_OBFUSCATE_RSSI            0
#endif

/**
 * @brief   Obfuscate max value
 */
#ifndef CONFIG_ED_BLE_OBFUSCATE_MAX
#define CONFIG_ED_BLE_OBFUSCATE_MAX             (100U)
#endif

/**
 * @brief   BLE RSSI transmission compensation gain
 */
#ifndef CONFIG_ED_BLE_TX_COMPENSATION_GAIN
#define CONFIG_ED_BLE_TX_COMPENSATION_GAIN      (0U)
#endif

/**
 * @brief    BLE RSSI receiption compensation gain
 */
#ifndef CONFIG_ED_BLE_RX_COMPENSATION_GAIN
#define CONFIG_ED_BLE_RX_COMPENSATION_GAIN      (0U)
#endif

/**
 * @brief   Values above this values will be clipped before being averaged
 */
#define RSSI_CLIPPING_THRESH    (0)


#if IS_USED(MODULE_ED_UWB)
/**
 * @brief   UWB encounter data, structure to track encounters per epoch
 */
typedef struct ed_uwb {
    uint32_t cumulative_d_cm;   /**< cumulative distance in cm */
    uint16_t req_count;         /**< request message count */
    uint16_t seen_first_s;      /**< time of first message, relative to start of epoch [s] */
    uint16_t seen_last_s;       /**< time of last message, relative to start of epoch [s] */
    bool valid;                 /**< valid encounter */
} ed_uwb_t;
#endif

#if IS_USED(MODULE_ED_BLE)
/**
 * @brief   BLE encounter data, structure to track encounters per epoch
 */
typedef struct ed_ble {
    float cumulative_rssi;      /**< cumulative distance rssi  */
    uint32_t cumulative_d_cm;   /**< cumulative distance in cm */
    uint16_t scan_count;        /**< scan count */
    uint16_t seen_first_s;      /**< time of first message, relative to start of epoch [s] */
    uint16_t seen_last_s;       /**< time of last message, relative to start of epoch [s] */
    bool valid;                 /**< valid encounter */
} ed_ble_t;
#endif

#if IS_USED(MODULE_ED_BLE_WIN)
/**
 * @brief   BLE Windowed encounter data, structure to track encounters per epoch
 */
typedef struct ed_ble_win {
    rdl_windows_t wins;         /**< windowed data, before @ref ed_finish is called
                                     the value will be the accumulated rssi sum, and after
                                     the end of an epoch @ed_finish is called and an averaged
                                     valued is computed */
    uint16_t seen_first_s;      /**< time of first message, relative to start of epoch [s] */
    uint16_t seen_last_s;       /**< time of last message, relative to start of epoch [s] */
    bool valid;                 /**< valid encounter */
} ed_ble_win_t;
#endif

/**
 * @brief   Encounter data, structure to track encounters per epoch
 */
typedef struct ed {
    clist_node_t list_node;     /**< list head */
    ebid_t ebid;                /**< the ebid structure */
#if IS_USED(MODULE_ED_BLE)
    ed_ble_t ble;
#endif
#if IS_USED(MODULE_ED_BLE_WIN)
    ed_ble_win_t ble_win;
#endif
#if IS_USED(MODULE_ED_UWB)
    ed_uwb_t uwb;
#endif
    uint32_t cid;               /**< the cid */
#if IS_USED(MODULE_ED_UWB) || IS_USED(MODULE_ED_BLE_WIN)
    int16_t obf;                 /**< obfuscation value or calibrated noise (CN) in DESIRE */
#endif
    uint16_t seen_last_s;       /**< time of last message, relative to start of epoch [s] */
} ed_t;

/**
 * @brief   UWB Encounter data memory manager structure
 */
typedef struct ed_memory_manager {
    uint8_t buf[CONFIG_ED_BUF_SIZE * sizeof(ed_t)]; /**< Task buffer */
    memarray_t mem;                                 /**< Memarray management */
} ed_memory_manager_t;

/**
 * @brief   UWB Encounter Data list type
 */
typedef struct ed_list {
    clist_node_t list;                  /**< list head */
    ed_memory_manager_t *manager;       /**< pointer to the encounter data memory manager */
    ebid_t *ebid;                       /**< pointer to the current epoch local ebid */
    uint32_t min_exposure_s;            /**< minimum exposure time in s */
} ed_list_t;

/**
 * @brief   Initialize an Encounter Data List
 *
 * @param[inout]    ed_list     the encounter data list to initialize
 * @param[in]       manager     the already initialized memory manager
 * @param[in]       ebid        the current epoch ebid
 */
static inline void ed_list_init(ed_list_t *ed_list,
                                ed_memory_manager_t *manager,
                                ebid_t *ebid)
{
    assert(ed_list);
    memset(ed_list, '\0', sizeof(ed_list_t));
    ed_list->list.next = NULL;
    ed_list->manager = manager;
    ed_list->ebid = ebid;
    ed_list->min_exposure_s = MIN_EXPOSURE_TIME_S;
}

/**
 * @brief   Initialize an Encounter Data List
 *
 * @param[inout]    ed_list         the encounter data list to initialize
 * @param[in]       min_expsure_s   the minimum exposure time for a valid encounter
 */
static inline void ed_list_set_min_exposure(ed_list_t *ed_list, uint32_t min_exposure_s)
{
    ed_list->min_exposure_s = min_exposure_s;
}

/**
 * @brief   Initialize an Encounter Data
 *
 * @param[inout]    ed          the encounter data element to initialize
 * @param[in]       cid         the conenction identifier
 */
static inline void ed_init(ed_t *ed, const uint32_t cid)
{
    assert(ed);
    memset(ed, '\0', sizeof(ed_t));
    ed->cid = cid;
}

/**
 * @brief   Retrieve cid based short address
 *
 * @param[inout]    ed          the encounter data element to initialize
 */
static inline uint16_t ed_get_short_addr(ed_t *ed)
{
    return (uint16_t)ed->cid;
}

/**
 * @brief   Add and encounter data to the list
 *
 * @note    Elements are assumed to be added to the list time ordered
 *
 * @param[in]       list       list to add element to
 * @param[in]       ed         element to add to list
 */
void ed_add(ed_list_t *list, ed_t *ed);

/**
 * @brief   Remove and EDL from the list
 *
 * @note    Due to the underlying list implementation, this will run in O(n).
 *
 * @param[in]       list       list to remove element from
 * @param[in]       ed         element to remove from list
 */
void ed_remove(ed_list_t *list, ed_t *ed);

/**
 * @brief   Find the an element in the list by cid
 *
 * @param[in]       list        the list to search the element in
 * @param[in]       cid         the cid to match
 *
 * @return          the found ed_t, NULL otherwise
 */
ed_t *ed_list_get_by_cid(ed_list_t *list, const uint32_t cid);

/**
 * @brief   Find the an element in the list by its short addres (lower 16bits of cid)
 *
 * @param[in]       list        the list to search the element in
 * @param[in]       addr        the addr to match
 *
 * @return          the found ed_t, NULL otherwise
 */
ed_t *ed_list_get_by_short_addr(ed_list_t *list, const uint16_t addr);

/**
 * @brief   Find the nth element in the list
 *
 * @note 0 is the first element added
 *
 * @param[in]       list        the list to search the element in
 * @param[in]       pos         position of element in list
 *
 * @return          the found ed_t, NULL otherwise
 */
ed_t *ed_list_get_nth(ed_list_t *list, int pos);

/**
 * @brief   Add ebid slice from advertisement data and attemp to reconstruct
 *
 * @note    Once a full EBID is received or reconstructed the start time for
 *          for the ed_t data is set to time.
 * @note    The third slice is expected to be front padded with
 *          @ref EBID_SLICE_SIZE_PAD '0'
 *
 * @param[in]       ed   the encounter matching the data
 * @param[in]       time     the timestamp in seconds relative to the start of
 *                           the epoch
 * @param[in]       slice    pointer to the advertised euid slice
 * @param[in]       part     the index of the slice to add
 *
 * @return  0 if whole ebid was just reconstructed
 * @return  1 if ebid is already reconstructed
 * @return  -1 otherwise
 */
int ed_add_slice(ed_t *ed, uint16_t time, const uint8_t *slice, uint8_t part);

#if IS_USED(MODULE_ED_UWB)
/**
 * @brief   Process additional data for an encounter extracted from an
 *          advertisement packet
 *
 * @param[in]       ed   the encounter matching the data
 * @param[in]       time     the timestamp in seconds relative to the start of
 *                           the epoch
 * @param[in]       d_cm     the distance of the device
 */
void ed_uwb_process_data(ed_t *ed, uint16_t time, uint16_t d_cm);
#endif

#if IS_USED(MODULE_ED_BLE)
/**
 * @brief   Process additional data for an encounter extracted from an
 *          advertisement packet
 *
 * @param[in]       ed       the encounter matching the data
 * @param[in]       time     the timestamp in seconds relative to the start of
 *                           the epoch
 * @param[in]       rssi     the rssi of the advertisement packet
 */
void ed_ble_process_data(ed_t *ed, uint16_t time, int8_t rssi);
#endif

#if IS_USED(MODULE_ED_BLE_WIN)
/** * @brief   Process additional data for an encounter extracted from an
 *             advertisement packet
 *
 * @param[in]       ed       the encounter matching the data
 * @param[in]       time     the timestamp in seconds relative to the start of
 *                           the epoch
 * @param[in]       rssi     the rssi of the advertisement packet
 */
void ed_ble_win_process_data(ed_t *ed, uint16_t time, int8_t rssi);
#endif

#if IS_USED(MODULE_ED_BLE) || IS_USED(MODULE_ED_BLE_WIN)
/**
 * @brief   Set the obfuscation value for the encounter data
 *
 * @pre     Both ebid must be full reconstructed, so ed->ebid and ebid
 *
 * @param[inout]    ed          the encounter data
 * @param[in]       ebid        the local ebid
 */
void ed_ble_set_obf_value(ed_t *ed, ebid_t *ebid);
#endif

/**
 * @brief   Process new data by adding it to the matching encounter data in an
 *          encounter data list.
 *
 * @note    If an encounter matching the ble address is not found then a new
 *          encounter_data entry will be added to the list as long as no more
 *          than @ref CONFIG_ED_BUF_SIZE are already tracked
 *
 * @param[in]       list     the encounter data list
 * @param[in]       cid      the connection identifier
 * @param[in]       time     the timestamp in seconds relative to the start of
 *                           the epoch
 * @param[in]       slice    pointer to the advertised euid slice
 * @param[in]       part     the index of the slice to add
 *
 * @return the found or allocated ed, NULL if none could be allocated
 */
ed_t *ed_list_process_slice(ed_list_t *list, const uint32_t cid, uint16_t time,
                            const uint8_t *slice, uint8_t part);

#if IS_USED(MODULE_ED_UWB)
/**
 * @brief   Process new data by adding it to the matching encounter data in an
 *          encounter data list.
 *
 * @note    If an encounter matching the ble address is not found then a new
 *          encounter_data entry will be added to the list as long as no more
 *          than @ref CONFIG_ED_BUF_SIZE are already tracked
 *
 * @param[in]       list     the encounter data list
 * @param[in]       addr     the the src addr
 * @param[in]       time     the timestamp in seconds relative to the start of
 *                           the epoch
 * @param[in]       d_cm     the measured distance in cm
 *
 * @return the found ed, NULL if no match
 */
ed_t *ed_list_process_rng_data(ed_list_t *list, const uint16_t addr, uint16_t time,
                               uint16_t d_cm);

void ed_serialize_uwb_json(uint16_t d_cm, uint32_t cid, uint32_t time, const char* base_name);
#endif

#if IS_USED(MODULE_ED_BLE) || IS_USED(MODULE_ED_BLE_WIN)
/**
 * @brief   Process new data by adding it to the matching encounter data in an
 *          encounter data list.
 *
 * @note    If an encounter matching the ble address is not found then a new
 *          encounter_data entry will be added to the list as long as no more
 *          than @ref CONFIG_ED_BUF_SIZE are already tracked
 *
 * @param[in]       list     the encounter data list
 * @param[in]       cid      the connection identifier
 * @param[in]       time     the timestamp in seconds relative to the start of
 *                           the epoch
 * @param[in]       rssi     the rssi of the advertisement packet
 *
 * @return the found ed, NULL if no match
 */
ed_t* ed_list_process_scan_data(ed_list_t *list, const uint32_t cid, uint16_t time,
                               int8_t rssi);

void ed_serialize_ble_json(int8_t rssi, uint32_t cid, uint32_t time, const char* base_name);

uint16_t ed_ble_rssi_to_cm(float rssi);
#endif

/**
 * @brief   Process all tracked encounters
 *
 * This will calculate the average rssi for all encounters in the list. If
 * the EBID cant be reconstructed or if the exposure time is not enough the
 * encounter is dropped from the list and freed.
 *
 * @param[in]      list     the encounter data list
 */
void ed_list_finish(ed_list_t *list);

/**
 * @brief   Finish processing of an encounter data
 *
 * @param[in]      list     the encounter data list
 * @param[in]      min_expsure_s   the minimum exposure time for a valid encounter
 *
 * @return         true if valid encounter, false otherwise (can be discarded)
 */
bool ed_finish(ed_t *ed, uint32_t min_exposure_s);

#if IS_USED(MODULE_ED_UWB)
/**
 * @brief   Process an encounter data
 *
 * This will calculate the average distance for the encounters in the list if
 * the EBID can be reconstructed and if the encounter time was enough.
 *
 * @param[in]      list     the encounter data list
 * @param[in]      min_expsure_s   the minimum exposure time for a valid encounter
 *
 * @return         true if valid encounter, false otherwise (can be discarded)
 */
bool ed_uwb_finish(ed_t *ed, uint32_t min_exposure_s);
#endif

#if IS_USED(MODULE_ED_BLE_WIN)
/**
 * @brief   Process an encounter data
 *
 * This will calculate the average RSSI if exposure time was enough.
 *
 * @param[in]      list     the encounter data list
 * @param[in]      min_expsure_s   the minimum exposure time for a valid encounter
 *
 * @return         true if valid encounter, false otherwise (can be discarded)
 */
bool ed_ble_win_finish(ed_t *ed, uint32_t min_exposure_s);
#endif

#if IS_USED(MODULE_ED_BLE)
/**
 * @brief   Process an encounter data
 *
 * This will calculate the average RSSI if exposure time was enough.
 *
 * @param[in]      list     the encounter data list
 * @param[in]      min_expsure_s   the minimum exposure time for a valid encounter
 *
 * @return         true if valid encounter, false otherwise (can be discarded)
 */
bool ed_ble_finish(ed_t *ed, uint32_t min_exposure_s);
#endif

/**
 * @brief   Init the memory manager
 *
 * @param[in]   manager     the memory manager structure to init
 */
void ed_memory_manager_init(ed_memory_manager_t *manager);

/**
 * @brief   Free an allocated element
 *
 * @param[in]       manager     the memory manager
 * @param[inout]    ed          the encounter data to to free
 */
void ed_memory_manager_free(ed_memory_manager_t *manager,
                            ed_t *ed);

/**
 * @brief   Allocate some space for a new encounter data entry
 *
 * @param[in]       manager     the memory manager
 *
 * @returns         pointer to the allocated encounter data structure
 */
ed_t *ed_memory_manager_calloc(ed_memory_manager_t *manager);

/**
 * @brief   Process an encounter data, data validaton is done in a VM
 *
 * This will calculate the average distance for the encounters in the list if
 * the EBID can be reconstructed and if the encounter time was enough.
 *
 * @param[in]      list     the encounter data list
 *
 * @return  true if valid encounter, false otherwise (can be discarded)
 */
bool ed_uwb_bpf_finish(ed_t *ed);

/**
 * @brief   Initiates UWB encounter data bpf handling
 *
 * This will setup storage used for bpf and a thread for suit updates
 */
void ed_uwb_bpf_init(void);

#include "board.h"
#include "periph/gpio.h"
/**
 * @brief   For 10s toggles a LED every 10ms
 */
void ed_blink_stop(gpio_t pin);
void ed_blink_start(gpio_t pin, uint32_t time_ms);

#ifdef __cplusplus
}
#endif

#endif /* UWB_ED_H */
/** @} */
