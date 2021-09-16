/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_uwb_ed Encounter Data List (EDL)
 * @ingroup     sys
 * @brief       Desire Encounter Data List
 *
 * Encounter data, rssi or range, is storuwb_ed as a timpuwb_ed stampuwb_ed linkuwb_ed list.
 * The linkuwb_ed list is assumuwb_ed to be accurately orderuwb_ed. If not it could be
 * orderuwb_ed by calling @ref clist_sort, but this consists of O(NlogN) runtime
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

#include "memarray.h"
#include "clist.h"
#include "ebid.h"

#include "bpf/uwb_ed_shared.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Size of the encounter data buffer, this gives an upper limit to
 *          the amount of encounters that can be tracked per epoch
 */
#ifndef CONFIG_UWB_ED_BUF_SIZE
#define CONFIG_UWB_ED_BUF_SIZE      (10U)
#endif

/**
 * @brief   UWB Encounter data, structure to track encounters per epoch
 */
typedef struct uwb_uwb_ed {
    clist_node_t list_node;     /**< list head */
    ebid_t ebid;                /**< the ebid structure */
    uint32_t cumulative_d_cm;   /**< cumulative distance in cm */
    uint32_t cid;               /**< the cid */
    uint16_t req_count;         /**< request message count */
    uint16_t seen_first_s;      /**< time of first message, relative to start of epoch [s] */
    uint16_t seen_last_s;       /**< time of last message, relative to start of epoch [s] */
} uwb_ed_t;

/**
 * @brief   UWB Encounter data memory manager structure
 */
typedef struct uwb_ed_memory_manager {
    uint8_t buf[CONFIG_UWB_ED_BUF_SIZE * sizeof(uwb_ed_t)]; /**< Task buffer */
    memarray_t mem;                                         /**< Memarray management */
} uwb_ed_memory_manager_t;

/**
 * @brief   UWB Encounter Data list type
 */
typedef struct uwb_uwb_ed_list {
    clist_node_t list;                  /**< list head */
    uwb_ed_memory_manager_t *manager;   /**< pointer to the encounter data memory manager */
    ebid_t *ebid;                       /**< pointer to the current epoch local ebid */
} uwb_ed_list_t;

/**
 * @brief   Initialize an Encounter Data List
 *
 * @param[inout]    uwb_ed_list     the encounter data list to initialize
 * @param[in]       manager     the already initialized memory manager
 * @param[in]       ebid        the current epoch ebid
 */
static inline void uwb_ed_list_init(uwb_ed_list_t *uwb_ed_list,
                                    uwb_ed_memory_manager_t *manager,
                                    ebid_t *ebid)
{
    assert(uwb_ed_list);
    memset(uwb_ed_list, '\0', sizeof(uwb_ed_list_t));
    uwb_ed_list->list.next = NULL;
    uwb_ed_list->manager = manager;
    uwb_ed_list->ebid = ebid;
}

/**
 * @brief   Initialize an Encounter Data
 *
 * @param[inout]    uwb_ed          the encounter data element to initialize
 * @param[in]       cid         the conenction identifier
 */
static inline void uwb_ed_init(uwb_ed_t *uwb_ed, const uint32_t cid)
{
    assert(uwb_ed);
    memset(uwb_ed, '\0', sizeof(uwb_ed_t));
    uwb_ed->cid = cid;
}

/**
 * @brief   Retrieve cid based short address
 *
 * @param[inout]    uwb_ed          the encounter data element to initialize
 */
static inline uint16_t uwb_get_short_addr(uwb_ed_t *uwb_ed)
{
    return (uint16_t)uwb_ed->cid;
}

/**
 * @brief   Add and encounter data to the list
 *
 * @note    Elements are assumed to be added to the list time ordered
 *
 * @param[in]       list       list to add element to
 * @param[in]       uwb_ed         element to add to list
 */
void uwb_ed_add(uwb_ed_list_t *list, uwb_ed_t *uwb_ed);

/**
 * @brief   Remove and EDL from the list
 *
 * @note    Due to the underlying list implementation, this will run in O(n).
 *
 * @param[in]       list       list to remove element from
 * @param[in]       uwb_ed         element to remove from list
 */
void uwb_ed_remove(uwb_ed_list_t *list, uwb_ed_t *uwb_ed);

/**
 * @brief   Find the an element in the list by cid
 *
 * @param[in]       list        the list to search the element in
 * @param[in]       cid         the cid to match
 *
 * @return          the found uwb_ed_t, NULL otherwise
 */
uwb_ed_t *uwb_ed_list_get_by_cid(uwb_ed_list_t *list, const uint32_t cid);

/**
 * @brief   Find the an element in the list by its short addres (lower 16bits of cid)
 *
 * @param[in]       list        the list to search the element in
 * @param[in]       addr        the addr to match
 *
 * @return          the found uwb_ed_t, NULL otherwise
 */
uwb_ed_t *uwb_ed_list_get_by_short_addr(uwb_ed_list_t *list,
                                        const uint16_t addr);

/**
 * @brief   Returns the exposure time for a given uwb_ed_t element
 *
 * @param[in]       uwb_ed        the element to calculate
 *
 * @return          the exposure time in seconds
 */
uint32_t uwb_ed_exposure_time(uwb_ed_t *uwb_ed);

/**
 * @brief   Add ebid slice from advertisement data and attemp to reconstruct
 *
 * @note    Once a full EBID is received or reconstructed the start time for
 *          for the uwb_ed_t data is set to time.
 * @note    The third slice is expected to be front padded with
 *          @ref EBID_SLICE_SIZE_PAD '0'
 *
 * @param[in]       uwb_ed   the encounter matching the data
 * @param[in]       time     the timestamp in seconds relative to the start of
 *                           the epoch
 * @param[in]       slice    pointer to the advertised euid slice
 * @param[in]       part     the index of the slice to add
 *
 * @return  0 if whole ebid has been received
 */
int uwb_ed_add_slice(uwb_ed_t *uwb_ed, uint16_t time, const uint8_t *slice,
                     uint8_t part);

/**
 * @brief   Process additional data for an encounter extracted from an
 *          advertisement packet
 *
 * @param[in]       uwb_ed   the encounter matching the data
 * @param[in]       time     the timestamp in seconds relative to the start of
 *                           the epoch
 * @param[in]       d_cm     the distance of the device
 */
void uwb_ed_process_data(uwb_ed_t *uwb_ed, uint16_t time, uint16_t d_cm);

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
 * @return the found or allocated uwb_ed, NULL if noce could be allocated
 */
uwb_ed_t *uwb_ed_list_process_slice(uwb_ed_list_t *list, const uint32_t cid,
                                    uint16_t time, const uint8_t *slice,
                                    uint8_t part);
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
 */

void uwb_ed_list_process_rng_data(uwb_ed_list_t *list, const uint16_t addr,
                                  uint16_t time, uint16_t d_cm);
/**
 * @brief   Process all tracked encounters
 *
 * This will calculate the average rssi for all encounters in the list. If
 * the EBID cant be reconstructed or if the exposure time is not enough the
 * encounter is dropped from the list and freed.
 *
 * @param[in]      list     the encounter data list
 */
void uwb_ed_list_finish(uwb_ed_list_t *list);

/**
 * @brief   Process an encounter data
 *
 * This will calculate the average distance for the encounters in the list if
 * the EBID can be reconstructed and if the encounter time was enough.
 *
 * @param[in]      list     the encounter data list
 * @return  true if valid encounter, false otherwise (can be discarded)
 */
bool uwb_ed_finish(uwb_ed_t *uwb_ed);

/**
 * @brief   Init the memory manager
 *
 * @param[in]   manager     the memory manager structure to init
 */
void uwb_ed_memory_manager_init(uwb_ed_memory_manager_t *manager);

/**
 * @brief   Free an allocated element
 *
 * @param[in]       manager     the memory manager
 * @param[inout]    uwb_ed          the encounter data to to free
 */
void uwb_ed_memory_manager_free(uwb_ed_memory_manager_t *manager,
                                uwb_ed_t *uwb_ed);

/**
 * @brief   Allocate some space for a new encounter data entry
 *
 * @param[in]       manager     the memory manager
 *
 * @returns         pointer to the allocated encounter data structure
 */
uwb_ed_t *uwb_ed_memory_manager_calloc(uwb_ed_memory_manager_t *manager);

/**
 * @brief   Process an encounter data, data validaton is done in a VM
 *
 * This will calculate the average distance for the encounters in the list if
 * the EBID can be reconstructed and if the encounter time was enough.
 *
 * @param[in]      list     the encounter data list
 * @return  true if valid encounter, false otherwise (can be discarded)
 */
bool uwb_ed_finish_bpf(uwb_ed_t *uwb_ed);

/**
 * @brief   Initiates UWB encounter data bpf handling
 *
 * This will setup storage used for bpf and a thread for suit updates
 */
void uwb_ed_bpf_init(void);

#ifdef __cplusplus
}
#endif

#endif /* UWB_ED_H */
/** @} */
