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

#ifndef ED_H
#define ED_H

#include <inttypes.h>
#include <assert.h>
#include <stdbool.h>

#include "memarray.h"

#include "ebid.h"
#include "clist.h"
#include "rdl_window.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Size of the ble advertisement address in bytes
 */
#define BLE_ADV_ADDR_SIZE       (6U)

/**
 * @brief   Size of the encounter data buffer, this gives an upper limit to
 *          the amount of encounters that can be tracked per epoch
 */
#ifndef CONFIG_ED_BUF_SIZE
#define CONFIG_ED_BUF_SIZE      (50U)
#endif

/**
 * @brief   Step between windows in seconds
 */
#ifndef MIN_EXPOSURE_TIME_S
#define MIN_EXPOSURE_TIME_S     (10 * 60LU)
#endif

/**
 * @brief   Encounter data, structure to track encounters per epoch
 */
typedef struct ed {
    clist_node_t list_node;     /**< list head */
    rdl_windows_t wins;         /**< windowed data, before @ref ed_finish is called
                                     the value will be the accumulated rssi sum, and after
                                     the end of an epoch @ed_finish is called and an averaged
                                     valued is computed */
    uint16_t start_s;           /**< time of first message, relative to start of epoch [s] */
    uint16_t end_s;             /**< time of last message, relative to start of epoch [s] */
    ebid_t ebid;                /**< the ebid structure */
    uint32_t cid;               /**< the cid */
} ed_t;

/**
 * @brief Encounter data memory manager structure
 */
typedef struct ed_memory_manager {
    uint8_t buf[CONFIG_ED_BUF_SIZE * sizeof(ed_t)]; /**< Task buffer */
    memarray_t mem;                                 /**< Memarray management */
} ed_memory_manager_t;

/**
 * @brief   Encounter Data list type
 */
typedef struct ed_list {
    clist_node_t list;              /**< list head */
    ed_memory_manager_t *manager;   /**< pointer to the encounter data memory manager */
} ed_list_t;

/**
 * @brief   Initialize an Encounter Data List
 *
 * @param[inout]    ed_list     the encounter data list to initialize
 * @param[in]       manafer     the already initialized memory manager
 */
static inline void ed_list_init(ed_list_t *ed_list,
                                ed_memory_manager_t *manager)
{
    assert(ed_list);
    memset(ed_list, '\0', sizeof(ed_list_t));
    ed_list->list.next = NULL;
    ed_list->manager = manager;
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
 * @brief   Find the an element in the list by cid
 *
 * @param[in]       list        the list to search the element in
 * @param[in]       cid         the cid to match
 *
 * @return          the found ed_t, NULL otherwise
 */
ed_t *ed_list_get_by_cid(ed_list_t *list, const uint32_t cid);

/**
 * @brief   Returns the exposure time for a given ed_t element
 *
 * @param[in]       ed        the element to calculate
 *
 * @return          the exposure time in seconds
 */
uint16_t ed_exposure_time(ed_t *ed);

/**
 * @brief   Process additional data for an encounter extracted from an
 *          advertisement packet
 *
 * @param[in]       ed       the encounter matching the data
 * @param[in]       time     the timestamp in seconds relative to the start of
 *                           the epoch
 * @param[in]       slice    pointer to the advertised euid slice
 * @param[in]       part     the index of the slice to add
 * @param[in]       rssi     the rssi of the advertisement packet
 */
void ed_process_data(ed_t *ed, uint16_t time, const uint8_t *slice,
                     uint8_t part, float rssi);

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
 * @param[in]       rssi     the rssi of the advertisement packet
 */
int ed_list_process_data(ed_list_t *list, const uint32_t cid, uint16_t time,
                         const uint8_t *slice, uint8_t part, float rssi);

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
 * @brief   Process an encounter data
 *
 * This will calculate the average rssi for the encounters in the list if
 * the EBID can be reconstructed and if the encounter time was enough.
 *
 * @param[in]      list     the encounter data list
 */
int ed_finish(ed_t *ed);

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
void ed_memory_manager_free(ed_memory_manager_t *manager, ed_t *ed);

/**
 * @brief   Allocate some space for a new encounter data entry
 *
 * @param[in]       manager     the memory manager
 *
 * @returns         pointer to the allocated encounter data structure
 */
ed_t *ed_memory_manager_calloc(ed_memory_manager_t *manager);

#ifdef __cplusplus
}
#endif

#endif /* ED_H */
/** @} */