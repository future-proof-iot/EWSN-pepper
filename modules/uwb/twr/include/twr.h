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
 *
 * ### Radio Sleeping
 *
 * Sleep handling could be improved by saving information on the next to trigger
 * event offset. I suspect that poor behaviour is because the radio is shutdown
 * just before it should be configured to rx/tx, this leads to delays that at
 * the end ruin synchronization.
 *
 * With information on the next to trigger two things can be done:
 *  - schedule the wakeup of the radio just before the event actually triggers
 *  - avoid putting the radio to sleep if an event is about to trigger
 *
 * Ideas:
 *  - something like `evtimer` but with `ZTIMER_MSEC_BASE`, why? Because using
 *    `ZTIMER_MSEC_BASE` gives more precise timestamping an accuracy for scheduled
 *     events.
 *  - add some kind of wakeup and sleep watchdog that are set/reset based on the
 *    radio status and the next to trigger event.
 *
 * #### Current status
 *
 * It is enabled with `twr_sleep` module. What is usually seen is that if there
 * are two nodes one will be able to perform ranging correctly while the second one
 * will fail.
 *
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
#ifndef CONFIG_TWR_PAN_ID
#define CONFIG_TWR_PAN_ID     (0xCAFE)
#endif
/**
 * @brief   The time window for which to switch on rng listening, max UINT16MAX
 *
 * @note    From experimental measurement the time between advertisement
 *          it ~600us, which means the start of the listening window can
 *          be off to upto 1.2ms without considering OS delays.
 */
#ifndef CONFIG_TWR_LISTEN_WINDOW_US
#define CONFIG_TWR_LISTEN_WINDOW_US     (2000U)
#endif
/**
 * @brief   TWR events to allocate, used to schedule rng_request/listen
 */
#ifndef CONFIG_TWR_EVENT_BUF_SIZE
#define CONFIG_TWR_EVENT_BUF_SIZE       (2 * 10)
#endif
/**
 * @brief   TWR rng_request default algorithm
 */
#ifndef CONFIG_TWR_EVENT_ALGO_DEFAULT
#define CONFIG_TWR_EVENT_ALGO_DEFAULT   (UWB_DATA_CODE_SS_TWR_ONE)
#endif

typedef enum {
    TWR_RNG_IDLE,
    TWR_RNG_INITIATOR,
    TWR_RNG_RESPONDER,
} twr_status_t;

/**
 * @brief   TWR event data type
 */
typedef struct twr_data {
    uint16_t addr;          /**< the neighbour address */
    uint16_t range;         /**< the range value in cm */
    uint16_t los;           /**< los as %*/
    uint32_t time;          /**< the measurement timestamp in msec */
} twr_event_data_t;

/**
 * @brief   TWR event type
 */
typedef struct twr_event {
    event_timeout_t timeout;    /**< the event timeout */
    event_callback_t event;     /**< the event callback */
    uint16_t addr;              /**< the address of destination */
} twr_event_t;

/**
 * @brief   Callback for ranging event notification
 */
typedef void (*twr_callback_t)(twr_event_data_t *, twr_status_t);

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
void twr_set_complete_cb(twr_callback_t callback);

/**
 * @brief   Register a callback to be called on RX timeout event
 *
 * @param[in]   callback    the callback
 */
void twr_set_rx_timeout_cb(twr_callback_t callback);

/**
 * @brief   Register a callback to be called when failed to schedule an event
 *
 * @param[in]   callback    the callback
 */
void twr_set_busy_cb(twr_callback_t callback);

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
 * @brief   Set the listen window
 *
 * @param[in]   time       listen window in us
 */
void twr_set_listen_window(uint16_t time);

/**
 * @brief   Return the listen window in us
 *
 * @return  listen window in us
 */
uint16_t twr_get_listen_window(void);

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
 *
 * @return  0 on success, -ENOMEM if no event available
 */
int twr_schedule_request_managed(uint16_t dest, uint16_t offset);

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
 *
 * @return  0 on success, -ENOMEM if no event available
 */
int twr_schedule_listen_managed(uint16_t addr, uint16_t offset);

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
/**
 * @brief   Set the memory manager for manged requests/listen events
 *
 * @returns         a pointer to the memory manager
 */
twr_event_mem_manager_t *twr_managed_get_manager(void);

/**
 * @brief   Enable TWR activity, enabled by default on init
 */
void twr_enable(void);

/**
 * @brief   Disable TWR activity, incoming event are ignored
 */
void twr_disable(void);

/**
 * @brief   Reset twr
 */
void twr_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* TWR_H */
/** @} */
