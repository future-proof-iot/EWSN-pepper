/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_edl Encounter Data List (EDL)
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

#ifndef EDL_H
#define EDL_H

#include <inttypes.h>
#include <assert.h>
#include <stdbool.h>

#include "clist.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Encounter Data Type uninitialized timestamp value
 */
#define EDL_TIMESTAMP_NONE      (UINT32_MAX)

/**
 * @brief   Values above this values will be clipped when added to the
 *          linked list
 */
#define EDL_DATA_RSS_CLIPPING_THRESH    (0)

/**
 * @brief   Encounter data union
 */
typedef union __attribute__((packed)) {
    int rssi;         /**< BLE rssi data */
    float range;      /**< TWR range data */
} edl_data_t;

/**
 * @brief   Encounter data list item
 */
typedef struct edl {
    clist_node_t list_node; /**< next element in list */
    edl_data_t data;        /**< encounter relevant data metrics */
    uint32_t timestamp;     /**< timestamp of data in ms*/
} edl_t;

/**
 * @brief   Encounter data list type
 */
typedef struct edl_list {
    clist_node_t list;    /**< list head */
    void* ebid;           /**< ebid pointer */
} edl_list_t;

/**
 * @brief   Initialize an Encounter Data List
 *
 * @param[inout]    edl_list    the encounter data list to initialize
 * @param[in]       ebid        pointer to the ebid matching the list
 */
static inline void edl_list_init(edl_list_t *edl_list, void* ebid)
{
    assert(edl_list && ebid);
    memset(edl_list, '\0', sizeof(edl_list_t));
    edl_list->list.next = NULL;
    edl_list->ebid = ebid;
}

/**
 * @brief   Initialize an Encounter Data List element
 *
 * @param[inout]    ed          the encounter data list element to initialize
 */
static inline void edl_init(edl_t * edl)
{
    assert(edl && ebid);
    memset(edl, '\0', sizeof(edl_t));
    edl->list_node.next = NULL;
    edl->timestamp = EDL_TIMESTAMP_NONE;
}

/**
 * @brief   Initialize an Encounter Data List element with a timestamp and rssi
 *
 * @param[inout]    ed          the encounter data list element to initialize
 * @param[in]       rssi        the element rssi
 * @param[in]       timestamp   the element timestamp
 */
static inline void edl_init_rssi(edl_t * edl, int rssi, uint32_t timestamp)
{
    edl_init(edl);
    if (rssi > EDL_DATA_RSS_CLIPPING_THRESH) {
        rssi = EDL_DATA_RSS_CLIPPING_THRESH;
    }
    edl->data.rssi = rssi;
    edl->timestamp = timestamp;
}

/**
 * @brief   Initialize an Encounter Data List element with a timestamp and range
 *
 * @param[inout]    ed          the encounter data list element to initialize
 * @param[in]       range       the element range
 * @param[in]       timestamp   the element timestamp
 */
static inline void edl_init_range(edl_t * edl, float range, uint32_t timestamp)
{
    edl_init(edl);
    edl->data.range = range;
    edl->timestamp = timestamp;
}

/**
 * @brief   ebd_list_t static initializer
 */
#define EDL_LIST_INIT (ebid)    { .ebid = ebid }

/**
 * @brief   Add and EDL from the list
 *
 * @note    Elements are assumed to be added to the list time ordered
 *
 * @param[in]       list        list to remove element from
 * @param[in]       edl         element to remove from list
 */
void edl_add(edl_list_t *list, edl_t* edl);

/**
 * @brief   Remove and EDL from the list
 *
 * @note    Due to the underlying list implementation, this will run in O(n).
 *
 * @param[in]       list        list to remove element from
 * @param[in]       edl         element to remove from list
 */
void edl_remove(edl_list_t *list, edl_t* edl);

/**
 * @brief   Find the nth element in the list
 *
 * @note 0 is the first element added
 *
 * @param[in]       list        the list to search the element in
 * @param[in]       pos         position of element in list
 *
 * @return          the found edl_t, NULL otherwise
 */
edl_t* edl_list_get_nth(edl_list_t *list, int pos);

/**
 * @brief   Returns first element which timestamp is before a given value
 *
 * @param[in]       list        the list to search the element in
 * @param[in]       before      time to compare to
 *
 * @return          the found edl_t, NULL otherwise
 */
edl_t* edl_list_get_before(edl_list_t *list, uint32_t before);

/**
 * @brief   Returns first element which timestamp is after a given value
 *
 * @param[in]       list        the list to search the element in
 * @param[in]       after       time to compare to
 *
 * @return          the found edl_t, NULL otherwise
 */
edl_t* edl_list_get_after(edl_list_t *list, uint32_t after);

/**
 * @brief   Returns first element which matches the given timestamp
 *
 * @param[in]       list        the list to search the element in
 * @param[in]       timestamp   time to compare to
 *
 * @return          the found edl_t, NULL otherwise
 */
edl_t* edl_list_get_by_time(edl_list_t *list, uint32_t timestamp);

/**
 * @brief   Returns the exposure time for a given list
 *
 * @param[in]       list        the list to calculate exposure
 *
 * @return          the exposure time
 */
uint32_t edl_list_exposure_time(edl_list_t *list);

/**
 * @brief   Returns the exposure time for a given list
 *
 * @param[in]       edl         encounter data list element
 * @param[in]       start       start of time range
 * @param[in]       end         end of time range
 *
 * @return          the exposure time
 */
bool edl_in_range(edl_t* edl, uint32_t start, uint32_t end);

#ifdef __cplusplus
}
#endif

#endif /* EDL_H */
/** @} */
