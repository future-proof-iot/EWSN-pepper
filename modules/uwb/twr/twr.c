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
#include <assert.h>
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
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

/* pointer to user set callback */
static twr_callback_t _usr_complete_cb = NULL;
static twr_callback_t _usr_rx_timeout_cb = NULL;
static twr_callback_t _usr_rx_cb = NULL;
static twr_callback_t _usr_busy_cb = NULL;
/* the event queue to offload to */
static event_queue_t *_twr_queue = NULL;

/* currently there can only be one so lets just keep a pointer to it */
struct uwb_rng_instance *_rng;
struct uwb_dev *_udev;

/* listening window duration in us */
static uint16_t listen_window_us = CONFIG_TWR_LISTEN_WINDOW_US;

/* mask TWR activity */
static bool _enabled = false;
/* status variable */
static twr_status_t _status = TWR_RNG_IDLE;
/* the present req dest short addr, or allowed short addr */
static uint16_t _other_short_addr;

/* memory manager pointer if any */
static twr_event_mem_manager_t *_manager = NULL;

static void _set_status_led(gpio_t pin, uint8_t state)
{
    if (IS_USED(MODULE_TWR_GPIO)) {
        if (gpio_is_valid(pin)) {
            gpio_write(pin, state);
        }
    }
}

/* */
static void _sleep_handler(event_t *event)
{
    (void)event;
    /* TODO: somehow save the offset of the next to be called scheduled event and
             only go to sleep if the next event is not close */
    if (IS_USED(MODULE_TWR_SLEEP)) {
        if (!_udev->status.sleeping) {
            uwb_sleep_config(_udev);
            uwb_enter_sleep(_udev);
        }
    }
}
static event_t _sleep_event = { .handler = _sleep_handler };

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

    if (IS_ACTIVE(CONFIG_DW1000_RX_DIAGNOSTIC)) {
        float rssi = uwb_calc_rssi(inst, inst->rxdiag);
        float fppl = uwb_calc_fppl(inst, inst->rxdiag);
        float los = DPL_FLOAT64_FROM_F32(
            uwb_estimate_los(rng->dev_inst, rssi, fppl));
        data.los = ((uint16_t)(los * 100));
        data.rssi = rssi;
    }
    /* TODO: can this be negative sometimes? */
    data.range = ((uint16_t)(range_f * 100));

    if (_usr_complete_cb == NULL) {
        LOG_DEBUG("[twr]: %" PRIu16 ", no usr callback\n", data.addr);
        LOG_DEBUG("\t - range: %" PRIu16 ".%" PRIu16 "\n",
                  (uint16_t)(data.range / 100U), (uint16_t)(data.range % 100U));
        if (IS_ACTIVE(CONFIG_DW1000_RX_DIAGNOSTIC)) {
            LOG_DEBUG("\t - los: %" PRIu16 ".%" PRIu16 "%%\n",
                      (uint16_t)(data.los / 100U), (uint16_t)(data.los % 100U));
        }
    }
    else {
        LOG_DEBUG("[twr]: %" PRIu32 ", calling usr callback\n", data.time);
        _usr_complete_cb(&data, _status);
    }
    _status = TWR_RNG_IDLE;
    return true;
}

/**
 * @brief api for receive timeout callback.
 *
 * @param[in] inst  pointer to struct uwb_dev.
 * @param[in] cbs   pointer to struct uwb_mac_interface.
 *
 * @return true on success
 */
static bool _rx_timeout_cb(struct uwb_dev *inst, struct uwb_mac_interface *cbs)
{
    (void)cbs;
    (void)inst;
    LOG_DEBUG("[twr]: rx_timeout 0x%04"PRIx16"\n", _other_short_addr);
    if (_usr_rx_timeout_cb) {
        twr_event_data_t data = { .addr = _other_short_addr };
        _usr_rx_timeout_cb(&data, _status);
    }
    event_post(_twr_queue, &_sleep_event);
    _status = TWR_RNG_IDLE;
    return true;
}

/* Structure of extension callbacks common for mac layer */
static struct uwb_mac_interface _uwb_mac_cbs = (struct uwb_mac_interface){
    .id = UWBEXT_APP0,
    .complete_cb = _complete_cb,
    .rx_timeout_cb = _rx_timeout_cb,
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

    /* set pan_id */
    twr_set_pan_id(CONFIG_TWR_PAN_ID);

    /* set state idle */
    _status = TWR_RNG_IDLE;
    if (IS_USED(MODULE_TWR_GPIO)) {
        if (gpio_is_valid(CONFIG_TWR_RESPONDER_PIN)) {
            gpio_init(CONFIG_TWR_RESPONDER_PIN, GPIO_OUT);
            gpio_set(CONFIG_TWR_RESPONDER_PIN);
        }
        if (gpio_is_valid(CONFIG_TWR_INITIATOR_PIN)) {
            gpio_init(CONFIG_TWR_INITIATOR_PIN, GPIO_OUT);
            gpio_set(CONFIG_TWR_INITIATOR_PIN);
        }
    }
    /* set event queue */
    _twr_queue = queue;
}

void twr_set_complete_cb(twr_callback_t callback)
{
    _usr_complete_cb = callback;
}

void twr_set_rx_timeout_cb(twr_callback_t callback)
{
    _usr_rx_timeout_cb = callback;
}

void twr_set_rx_cb(twr_callback_t callback)
{
    _usr_rx_cb = callback;
}

void twr_set_busy_cb(twr_callback_t callback)
{
    _usr_busy_cb = callback;
}

void twr_set_listen_window(uint16_t time)
{
    listen_window_us = time;
}

uint16_t twr_get_listen_window(void)
{
    return listen_window_us;
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
    uwb_set_panid(_udev, _udev->pan_id);
}

static void _twr_rng_listen(void *arg)
{
    (void)arg;
    twr_event_t *event = (twr_event_t *)arg;

    if (_enabled) {
        if (dpl_sem_get_count(&_rng->sem) == 1) {
            LOG_DEBUG("[twr]: rng listen start\n");
            _set_status_led(CONFIG_TWR_RESPONDER_PIN, 1);
            if (IS_USED(MODULE_TWR_SLEEP) && _udev->status.sleeping) {
                uwb_wakeup(_udev);
            }
            _status = TWR_RNG_RESPONDER;
            _other_short_addr = event->addr;
            struct uwb_dev_status status = uwb_rng_listen(_rng, listen_window_us, UWB_BLOCKING);
            if (!status.rx_error && !status.rx_timeout_error && !status.start_rx_error) {
                LOG_DEBUG("[twr]: rng listen OK\n");
                twr_event_data_t data = { .addr = event->addr };
                _usr_rx_cb(&data, TWR_RNG_RESPONDER);
            }
            _set_status_led(CONFIG_TWR_RESPONDER_PIN, 0);
            event_post(_twr_queue, &_sleep_event);
            return;
        }
        else {
            LOG_WARNING("[twr]: rng listen aborted, busy\n");
        }
    }
    else {
        LOG_DEBUG("[twr]: skip, is disabled\n");
    }
    if (_usr_busy_cb) {
        twr_event_data_t data = { .addr = event->addr };
        _usr_busy_cb(&data, TWR_RNG_RESPONDER);
    }
}

static void _twr_rng_listen_managed(void *arg)
{
    twr_event_t *event = (twr_event_t *)arg;

    _twr_rng_listen(event);
    twr_event_mem_manager_free(_manager, (twr_event_t *)arg);
}

static void _twr_schedule_listen(twr_event_t *event, uint16_t offset, void (*callback)(void *))
{
    event_callback_init(&event->event, callback, event);
    event_timeout_ztimer_init(&event->timeout, ZTIMER_MSEC_BASE, _twr_queue, &event->event.super);
    event_timeout_set(&event->timeout, offset);
    LOG_DEBUG("[twr]: schedule rng listen in %" PRIu16 "\n", offset);
}

void twr_schedule_listen(twr_event_t *event, uint16_t offset)
{
    _twr_schedule_listen(event, offset, _twr_rng_listen);
}

int twr_schedule_listen_managed(uint16_t addr, uint16_t offset)
{
    assert(_manager);
    twr_event_t *event = twr_event_mem_manager_calloc(_manager);

    event->addr = addr;

    if (!event) {
        LOG_ERROR("[twr]: error, no lst event\n");
        if (IS_ACTIVE(CONFIG_TWR_RESET_ON_LOCK)) {
            twr_reset();
        }
        return -ENOMEM;
    }
    _twr_schedule_listen(event, offset, _twr_rng_listen_managed);
    return 0;
}

static void _twr_rng_request(void *arg)
{
    (void) arg;
    twr_event_t *event = (twr_event_t *)arg;

    if (_enabled) {
        if (dpl_sem_get_count(&_rng->sem) == 1) {
            LOG_DEBUG("[twr]: rng request to %4" PRIx16 "\n", event->addr);
            _other_short_addr = event->addr;
            /* wake up if needed */
            _set_status_led(CONFIG_TWR_INITIATOR_PIN, 1);
            if (IS_USED(MODULE_TWR_SLEEP) && _udev->status.sleeping) {
                uwb_wakeup(_udev);
            }
            _status = TWR_RNG_INITIATOR;
            uwb_rng_request(_rng, event->addr, CONFIG_TWR_EVENT_ALGO_DEFAULT);
            _set_status_led(CONFIG_TWR_INITIATOR_PIN, 0);
            event_post(_twr_queue, &_sleep_event);
            return;
        }
        else {
            LOG_WARNING("[twr]: rng request aborted, busy\n");
        }
    }
    else {
        LOG_DEBUG("[twr]: skip, is disabled\n");
    }
    if (_usr_busy_cb) {
        twr_event_data_t data = { .addr = event->addr };
        _usr_busy_cb(&data, TWR_RNG_INITIATOR);
    }
}

static void _twr_rng_request_managed(void *arg)
{
    twr_event_t *event = (twr_event_t *)arg;

    _twr_rng_request(event);
    twr_event_mem_manager_free(_manager, (twr_event_t *)arg);
}

static void _twr_schedule_request(twr_event_t *event, uint16_t dest, uint16_t offset,
                                  void (*callback)(void *))
{
    event->addr = dest;
    event_callback_init(&event->event, callback, event);
    event_timeout_ztimer_init(&event->timeout, ZTIMER_MSEC_BASE, _twr_queue, &event->event.super);
    event_timeout_set(&event->timeout, offset);
    LOG_DEBUG("[twr]: schedule rng request to %4" PRIx16 " in %" PRIu16 "\n",
              event->addr, offset);
}

void twr_schedule_request(twr_event_t *event, uint16_t dest, uint16_t offset)
{
    _twr_schedule_request(event, dest, offset, _twr_rng_request);
}

int twr_schedule_request_managed(uint16_t dest, uint16_t offset)
{
    assert(_manager);
    twr_event_t *event = twr_event_mem_manager_calloc(_manager);

    if (!event) {
        LOG_ERROR("[twr]: error, no req event\n");
        if (IS_ACTIVE(CONFIG_TWR_RESET_ON_LOCK)) {
            twr_reset();
        }
        return -ENOMEM;
    }
    _twr_schedule_request(event, dest, offset, _twr_rng_request_managed);
    return 0;
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

twr_event_mem_manager_t *twr_managed_get_manager(void)
{
    return _manager;
}

void twr_enable(void)
{
    _enabled = true;
    if (IS_USED(MODULE_TWR_SLEEP) && _udev->status.sleeping) {
        uwb_wakeup(_udev);
        uwb_phy_forcetrxoff(_udev);
    }
}

void twr_disable(void)
{
    _enabled = false;
    _status = TWR_RNG_IDLE;
    /* TODO: this should make sure that at least at the end of an epoch all twr
       event get releases: it would be important to log when it happens... */
    uwb_phy_forcetrxoff(_udev);

    if (IS_USED(MODULE_TWR_SLEEP) && !_udev->status.sleeping) {
        uwb_sleep_config(_udev);
        uwb_enter_sleep(_udev);
    }
}

void twr_reset(void)
{
    _status = TWR_RNG_IDLE;
    uwb_phy_forcetrxoff(_udev);
    if (dpl_sem_get_count(&_rng->sem) == 0) {
        dpl_error_t err = dpl_sem_release(&_rng->sem);
        assert(err == (dpl_error_t)DPL_OK);
    }
}
