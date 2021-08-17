/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_twr
 * @{
 *
 * @file
 * @brief       Two Way Ranging implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */
#include <errno.h>

#include "twr.h"
#include "uwb/uwb.h"
#include "uwb_rng/uwb_rng.h"

#include "timex.h"
#include "ztimer.h"
#include "event.h"
#include "event/callback.h"
#include "event/timeout.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

/* pointer to user set callback */
static twr_callback_t _usr_rng_callback = NULL;
/* the event queue to offload to */
static event_queue_t *_twr_queue = NULL;

/* currently there can only be one so lets just keep a pointer to it */
struct uwb_rng_instance *_rng;
struct uwb_dev *_udev;

/* memory manager pointer if any */
twr_event_mem_manager_t *_manager = NULL;

/**
 * @brief Range request complete callback.
 *
 * This callback is called on completion of a range request in the
 * context of this example.
 *
 * @note This runs in ISR context, offload as much as possible to a
 * thread/event queue.
 *
 * @note The MAC extension interface is a linked-list of callbacks,
 * subsequentcallbacks on the list will be not be called if the callback
 * returns true.
 *
 * @param[in] inst  Pointer to struct uwb_dev.
 * @param[in] cbs   Pointer to struct uwb_mac_interface.
 *
 * @return true if valid recipient for the event, false otherwise
 */
static bool _complete_cb(struct uwb_dev *inst, struct uwb_mac_interface *cbs)
{
    twr_event_data_t data;
    twr_frame_t *frame;

    if (inst->fctrl != FCNTL_IEEE_RANGE_16 &&
        inst->fctrl != (FCNTL_IEEE_RANGE_16 | UWB_FCTRL_ACK_REQUESTED)) {
        return false;
    }

    /* timestamp */
    uint32_t now = ztimer_now(ZTIMER_MSEC);

    /* get received frame and parse data*/
    struct uwb_rng_instance *rng = (struct uwb_rng_instance *)cbs->inst_ptr;
    rng->idx_current = (rng->idx) % rng->nframes;
    frame = rng->frames[rng->idx_current];
    if (frame->src_address == _udev->my_short_address) {
        data.addr = frame->dst_address;
    }
    else {
        data.addr = frame->src_address;
    }
    data.time = now;
    float range_f =
        uwb_rng_tof_to_meters(uwb_rng_twr_to_tof(rng, rng->idx_current));
    /* TODO: can this be negative sometimes? */
    data.range = ((uint16_t)(range_f * 100));

    if (_usr_rng_callback == NULL) {
        LOG_DEBUG("[twr]: %" PRIu16 ", no usr callback\n", data.addr);
        LOG_DEBUG("\t - range: %" PRIu16 ".%" PRIu16 "\n",
                  (uint16_t)(data.range / 100U), (uint16_t)(data.range % 100U));
        LOG_DEBUG("\t - time: %" PRIu32 "\n", data.time);
    }
    else {
        LOG_DEBUG("[twr]: %" PRIu32 ", calling usr callback\n", data.time);
        _usr_rng_callback(&data);
    }

    return true;
}

/**
 * @brief API for receive timeout callback.
 *
 * @param[in] inst  Pointer to struct uwb_dev.
 * @param[in] cbs   Pointer to struct uwb_mac_interface.
 *
 * @return true on success
 */
static bool _rx_timeout_cb(struct uwb_dev *inst, struct uwb_mac_interface *cbs)
{
    (void)cbs;
    (void)inst;
    LOG_DEBUG("[twr]: rx_timeout\n");
    return true;
}

/* Structure of extension callbacks common for mac layer */
static struct uwb_mac_interface _uwb_mac_cbs = (struct uwb_mac_interface){
    .id = UWBEXT_APP0,
    .complete_cb = _complete_cb,
    .rx_timeout_cb = _rx_timeout_cb
};

void twr_init(event_queue_t *queue)
{
    /* recover the instance pointer, only one is currently supported */
    struct uwb_dev *udev = uwb_dev_idx_lookup(0);
    struct uwb_rng_instance *rng =
        (struct uwb_rng_instance *)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_RNG);

    assert(rng);
    _rng = rng;
    _udev = udev;

    /* set up local mac callbacks */
    _uwb_mac_cbs.inst_ptr = rng;
    uwb_mac_append_interface(udev, &_uwb_mac_cbs);

    if (IS_USED(MODULE_UWB_CORE_TWR_DS_ACK)) {
        uwb_set_autoack(udev, true);
        uwb_set_autoack_delay(udev, 12);
    }

    /* apply config */
    uwb_mac_config(udev, NULL);
    uwb_txrf_config(udev, &udev->config.txrf);
    /* set pan_id */
    twr_set_pan_id(CONFIG_TWR_PAN_ID);

    /* set event queue */
    _twr_queue = queue;
}

void twr_register_rng_cb(twr_callback_t callback)
{
    _usr_rng_callback = callback;
}

void twr_set_short_addr(uint16_t address)
{
    _udev->my_short_address = address;
    LOG_DEBUG("[twr]: setting short address to %4" PRIx16 "\n", address);
    uwb_set_uid(_udev, _udev->my_short_address);
}

void twr_set_pan_id(uint16_t pan_id)
{
    _udev->pan_id = pan_id;
    uwb_set_uid(_udev, _udev->pan_id);
}

static void _twr_rng_listen(void *arg)
{
    (void)arg;
    if (dpl_sem_get_count(&_rng->sem) == 1) {
        LOG_DEBUG("[twr]: rng listen start\n");
        uwb_rng_listen(_rng, CONFIG_TWR_LISTEN_WINDOW_MS * US_PER_MS,
                       UWB_NONBLOCKING);
    }
    else {
        LOG_ERROR("[twr]: rng listen aborted, busy\n");
    }
}

static void _twr_rng_listen_managed(void *arg)
{
    _twr_rng_listen(arg);
    twr_event_mem_manager_free(_manager, (twr_event_t *)arg);
}

static void _twr_schedule_listen(twr_event_t *event, uint16_t offset, void (*callback)(void *))
{
    event_callback_init(&event->event, callback, event);
    LOG_DEBUG("[twr]: schedule rng listen in %" PRIu16 "\n", offset);
    event_timeout_ztimer_init(&event->timeout, ZTIMER_MSEC, _twr_queue,
                              &event->event.super);
    event_timeout_set(&event->timeout, offset);
}

void twr_schedule_listen(twr_event_t *event, uint16_t offset)
{
    _twr_schedule_listen(event, offset, _twr_rng_listen);
}

void twr_schedule_listen_managed(uint16_t offset)
{
    assert(_manager);
    twr_event_t *event = twr_event_mem_manager_calloc(_manager);
    if (!event) {
        LOG_ERROR("[twr]: error, no lst event\n");
        return;
    }
    _twr_schedule_listen(event, offset, _twr_rng_listen_managed);
}

static void _twr_rng_request(void *arg)
{
    twr_event_t *event = (twr_event_t *)arg;

    if (dpl_sem_get_count(&_rng->sem) == 1) {
        LOG_DEBUG("[twr]: rng request to %4" PRIx16 "\n", event->addr);
        uwb_rng_request(_rng, event->addr, CONFIG_TWR_EVENT_ALGO_DEFAULT);
    }
    else {
        LOG_ERROR("[twr]: rng request aborted, busy\n");
    }
}

static void _twr_rng_request_managed(void *arg)
{
    _twr_rng_request(arg);
    twr_event_mem_manager_free(_manager, (twr_event_t *)arg);
}

static void _twr_schedule_request(twr_event_t *event, uint16_t dest, uint16_t offset,
                                  void (*callback)(void *))
{
    event->addr = dest;
    LOG_DEBUG("[twr]: schedule rng request to %4" PRIx16 " in %" PRIu16 "\n",
              event->addr, offset);
    event_callback_init(&event->event, callback, event);
    event_timeout_ztimer_init(&event->timeout, ZTIMER_MSEC, _twr_queue,
                              &event->event.super);
    event_timeout_set(&event->timeout, offset);
}

void twr_schedule_request(twr_event_t *event, uint16_t dest, uint16_t offset)
{
    _twr_schedule_request(event, dest, offset, _twr_rng_request);
}

void twr_schedule_request_managed(uint16_t dest, uint16_t offset)
{
    assert(_manager);
    twr_event_t *event = twr_event_mem_manager_calloc(_manager);

    if (!event) {
        LOG_ERROR("[twr]: error, no req event\n");
        return;
    }
    _twr_schedule_request(event, dest, offset, _twr_rng_request_managed);
}

void twr_event_mem_manager_init(twr_event_mem_manager_t *manager)
{
    memset(manager, '\0', sizeof(twr_event_mem_manager_t));
    memarray_init(&manager->mem, manager->buf, sizeof(twr_event_t),
                  CONFIG_TWR_EVENT_BUF_SIZE);
}

void twr_event_mem_manager_free(twr_event_mem_manager_t *manager,
                                twr_event_t *event)
{
    memarray_free(&manager->mem, event);
}

twr_event_t *twr_event_mem_manager_calloc(twr_event_mem_manager_t *manager)
{
    return memarray_calloc(&manager->mem);
}

void twr_managed_set_manager(twr_event_mem_manager_t *manager)
{
    _manager = manager;
}
