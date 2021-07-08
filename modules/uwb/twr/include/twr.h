/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_twr Two Way Ranging
 * @ingroup     sys
 * @brief       Two Way Ranging Module
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef TWR_H
#define TWR_H

#include "memarray.h"
#include "event.h"
#include "event/timeout.h"
#include "event/callback.h"
#include "uwb/uwb_ftypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   The time window for witch to switch on rng listening
 */
#ifndef CONFIG_TWR_LISTEN_WINDOW_MS
#define CONFIG_TWR_LISTEN_WINDOW_MS     (6)
#endif
/**
 * @brief   TWR events to allocate, used to schedule rng_request/listen
 */
#ifndef CONFIG_TWR_EVENT_BUF_SIZE
#define CONFIG_TWR_EVENT_BUF_SIZE       (2 * 20)
#endif
/**
 * @brief   TWR rng_request default algorithm
 */
#ifndef CONFIG_TWR_EVENT_ALGO_DEFAULT
#define CONFIG_TWR_EVENT_ALGO_DEFAULT   (UWB_DATA_CODE_SS_TWR)
#endif

/**
 * @brief   TWR event data type
 */
typedef struct twr_data {
    uint16_t addr;          /**< the neighbour address */
    uint16_t range;         /**< the range value in cm */
    uint32_t time;          /**< the measurement timestamp in msec */
} twr_event_data_t;

/**
 * @brief   TWR event type
 */
typedef struct twr_req_event {
    event_timeout_t timeout;    /**< the event timeout */
    event_callback_t event;     /**< the event callback */
    uint16_t addr;              /**< the address of destination */
} twr_event_t;

/**
 * @brief   Callback for ranging event notification
 */
typedef void (*twr_callback_t)(twr_event_data_t *);

/**
 * @brief
 */
typedef struct twr_event_mem_manager {
    uint8_t buf[CONFIG_TWR_EVENT_BUF_SIZE * sizeof(twr_event_t)];   /**< event buffer */
    memarray_t mem;                                                 /**< Memarray management */
} twr_event_mem_manager_t;

/**
 * @brief   Initialize twr and set the event queue
 *
 * @param[in]   queue       the event queue
 */
void twr_init(event_queue_t *queue);

/**
 * @brief   Register a callback to be called on a complete TWR exchange
 *
 * @param[in]   callback    the callback
 */
void twr_register_rng_cb(twr_callback_t callback);

/**
 * @brief   Set the UWB device short address
 *
 * @param[in]   address     the address
 */
void twr_set_short_addr(uint16_t address);

/**
 * @brief   Set the UWB device PAN ID
 *
 * @param[in]   pan_id     the pan_id
 */
void twr_set_pan_id(uint16_t pan_id);

/**
 * @brief   Schedule a uwb_rng_request at an offset
 *
 * @param[inout]    event   the pre-allocated event to post
 * @param[in]       dest    the destination short address
 * @param[in]       offset  the time offset at witch to send the rng_request
 */
void twr_schedule_request(twr_event_t *event, uint16_t dest, uint16_t offset);

/**
 * @brief   Schedule a uwb_rng_request at an offset, event are allocated from memarray
 *
 * @pre     A initialized @ref twr_event_mem_manager, and twr_managed_set_manager()
 *
 * @param[in]       dest    the destination short address
 * @param[in]       offset  the time offset at witch to send the rng_request
 */
void twr_schedule_request_managed(uint16_t dest, uint16_t offset);

/**
 *
 * @brief   Schedule a uwb_rng_listen at an offset
 *
 * @param[inout]    event   the pre-allocated event to post
 * @param[in]       offset  the time offset at witch to start listening
 */
void twr_schedule_listen(twr_event_t *event, uint16_t offset);

/**
 *
 * @brief   Schedule a uwb_rng_listen at an offset
 *
 * @pre     A initialized @ref twr_event_mem_manager, and twr_managed_set_manager()
 *
 * @param[in]       offset  the time offset at witch to start listening
 */
void twr_schedule_listen_managed(uint16_t offset);

/**
 * @brief   Init the memory manager
 *
 * @param[in]   manager     the memory manager structure to init
 */
void twr_event_mem_manager_init(twr_event_mem_manager_t *manager);

/**
 * @brief   Free an allocated element
 *
 * @param[in]       manager     the memory manager
 * @param[inout]    event       the event to to free
 */
void twr_event_mem_manager_free(twr_event_mem_manager_t *manager,
                                twr_event_t *event);

/**
 * @brief   Allocate some space for a twr event
 *
 * @param[in]       manager     the memory manager
 *
 * @returns         pointer to the allocated event
 */
twr_event_t *twr_event_mem_manager_calloc(twr_event_mem_manager_t *manager);

/**
 * @brief   Set the memory manager for manged requests/listen events
 *
 * @param[in]       manager     the memory manager
 */
void twr_managed_set_manager(twr_event_mem_manager_t *manager);

#ifdef __cplusplus
}
#endif

#endif /* TWR_H */
/** @} */