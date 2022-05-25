#include "pepper_srv.h"
#include "event.h"
#include "event/callback.h"
#include "event/thread.h"
#include "event/timeout.h"
#include "mutex.h"
#include "xfa.h"
#include "ztimer.h"
#include "timex.h"
#include "random.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

XFA_INIT_CONST(pepper_srv_endpoint_t, pepper_srv_endpoints);

static uint8_t number_endpoints = 0;
static epoch_data_t _epoch_data;
static ed_uwb_data_t _uwb_data;
static ed_ble_data_t _ble_data;
static mutex_t _epoch_lock = MUTEX_INIT;
static mutex_t _uwb_lock = MUTEX_INIT;
static mutex_t _ble_lock = MUTEX_INIT;
static event_queue_t *_evt_queue = NULL;
static event_timeout_t _notify_epoch_timeout;
static event_timeout_t _notify_uwb_timeout;
static event_timeout_t _notify_ble_timeout;

static void _set_status_led(gpio_t pin, uint8_t state)
{
    if (IS_USED(MODULE_PEPPER_SRV_LEDS)) {
        if (gpio_is_valid(pin)) {
            gpio_write(pin, state);
        }
    }
}

/* callback to notify new epoch_data */
static void _notify_epoch_data(void *arg)
{
    epoch_data_t *data = (epoch_data_t *)arg;

    pepper_srv_notify_epoch_data(data);
}
static event_callback_t _epoch_data_submit_event = EVENT_CALLBACK_INIT(
    _notify_epoch_data, &_epoch_data);

void pepper_srv_data_submit(epoch_data_t *data)
{
    /* try to acquire the lock, if failed, just drop the data */
    if (mutex_trylock(&_epoch_lock)) {
        memcpy(&_epoch_data, data, sizeof(epoch_data_t));
        event_timeout_set(&_notify_epoch_timeout, MS_PER_SEC * random_uint32_range(5, 10));
    }
    else {
        LOG_WARNING("[pepper_srv]: dropped epoch data\n");
    }
}

/* callback to notify new uwb_data */
static void _notify_uwb_data(void *arg)
{
    ed_uwb_data_t *data = (ed_uwb_data_t *)arg;

    pepper_srv_notify_uwb_data(data);
}
static event_callback_t _uwb_data_submit_event = EVENT_CALLBACK_INIT(
    _notify_uwb_data, &_uwb_data);

void pepper_srv_uwb_data_submit(ed_uwb_data_t *data)
{
    /* try to acquire the lock, if failed, just drop the data */
    if (mutex_trylock(&_uwb_lock)) {
        memcpy(&_uwb_data, data, sizeof(ed_uwb_data_t));
        event_timeout_set(&_notify_uwb_timeout, random_uint32_range(5, 10));
    }
    else {
        printf("[pepper_srv]: dropped uwb data\n");
    }
}

/* callback to notify new ble_data */
static void _notify_ble_data(void *arg)
{
    ed_ble_data_t *data = (ed_ble_data_t *)arg;

    pepper_srv_notify_ble_data(data);
}
static event_callback_t _ble_data_submit_event = EVENT_CALLBACK_INIT(
    _notify_ble_data, &_ble_data);

void pepper_srv_ble_data_submit(ed_ble_data_t *data)
{
    /* try to acquire the lock, if failed, just drop the data */
    if (mutex_trylock(&_ble_lock)) {
        memcpy(&_ble_data, data, sizeof(ed_ble_data_t));
        event_timeout_set(&_notify_ble_timeout, random_uint32_range(5, 10));
    }
    else {
        printf("[pepper_srv]: dropped ble data\n");
    }
}

/* init : init all endpoints and core */
int pepper_srv_init(event_queue_t *evt_queue)
{
    int ret = 0;

    LOG_INFO("[pepper_srv]: init\n");
    number_endpoints = XFA_LEN(pepper_srv_endpoint_t, pepper_srv_endpoints);

    _evt_queue = evt_queue; /* keep for internal event handling */
    event_timeout_ztimer_init(&_notify_epoch_timeout, ZTIMER_MSEC, _evt_queue,
                              &_epoch_data_submit_event.super);
    event_timeout_ztimer_init(&_notify_uwb_timeout, ZTIMER_MSEC, _evt_queue,
                              &_uwb_data_submit_event.super);
    event_timeout_ztimer_init(&_notify_ble_timeout, ZTIMER_MSEC, _evt_queue,
                              &_ble_data_submit_event.super);

    LOG_INFO("[pepper_srv]: number_endpoints %d\n", number_endpoints);
    for (uint8_t i = 0; i < number_endpoints; i++) {
        ret = pepper_srv_endpoints[i].init(evt_queue);
        if (ret) {
            ret |= (1 << i);
        }
    }
    return ret;
}

/* notify (non-blocking) : add to ring buffer and notify all endpoints
   (post event handler for doing this)*/
int pepper_srv_notify_epoch_data(epoch_data_t *epoch_data)
{
    int ret = 0;

    /* Blocking calls : TODO check if useful to do it in async way by posting
       event handler(s) */
    for (uint8_t i = 0; i < number_endpoints; i++) {
        if (pepper_srv_endpoints[i].notify_epoch_data) {
            ret = pepper_srv_endpoints[i].notify_epoch_data(epoch_data);
            if (ret) {
                ret |= (1 << i);
            }
        }
    }
    /* unlock epoch_data lock, allowing for new data to be submitted */
    mutex_unlock(&_epoch_lock);
    return ret;
}

/* notify (non-blocking) : add to ring buffer and notify all endpoints
   (post event handler for doing this)*/
int pepper_srv_notify_uwb_data(ed_uwb_data_t *data)
{
    int ret = 0;

    LOG_INFO("[pepper_srv]: number_endpoints %d\n", number_endpoints);
    /* Blocking calls : TODO check if useful to do it in async way by posting
       event handler(s) */
    for (uint8_t i = 0; i < number_endpoints; i++) {
        if (pepper_srv_endpoints[i].notify_uwb_data) {
            ret = pepper_srv_endpoints[i].notify_uwb_data(data);
            if (ret) {
                ret |= (1 << i);
            }
        }
    }
    /* unlock uwb_data lock, allowing for new data to be submitted */
    mutex_unlock(&_uwb_lock);
    return ret;
}

/* notify (non-blocking) : add to ring buffer and notify all endpoints
   (post event handler for doing this)*/
int pepper_srv_notify_ble_data(ed_ble_data_t *data)
{
    int ret = 0;

    LOG_INFO("[pepper_srv]: number_endpoints %d\n", number_endpoints);
    /* Blocking calls : TODO check if useful to do it in async way by posting
       event handler(s) */
    for (uint8_t i = 0; i < number_endpoints; i++) {
        if (pepper_srv_endpoints[i].notify_ble_data) {
            ret = pepper_srv_endpoints[i].notify_ble_data(data);
            if (ret) {
                ret |= (1 << i);
            }
        }
    }
    /* unlock ble_data lock, allowing for new data to be submitted */
    mutex_unlock(&_ble_lock);
    return ret;
}

/* Blocking call (?) : iterate on all endpoints for notifying them about infection */
int pepper_srv_notify_infection(bool infected)
{
    int ret = 0;

    if (infected) {
        _set_status_led(CONFIG_PEPPER_SRV_INF_LED, 0);
    }
    else {
        _set_status_led(CONFIG_PEPPER_SRV_INF_LED, 1);
    }

    /* Blocking calls : TODO check if useful to do it in async way by posting
       event handler(s) */
    for (uint8_t i = 0; i <  number_endpoints; i++) {
        if (pepper_srv_endpoints[i].notify_infection) {
            ret = pepper_srv_endpoints[i].notify_infection(infected);
            if (ret) {
                ret |= (1 << i);
            }
        }
    }
    return ret;
}

/* Blocking call (?) : iterate on all endpoints for quering infection, retrun 0
   if none of endpoints returns true, otherwise a bitmap with an exposure flag
   per endpoint */
int pepper_srv_esr(bool *esr /*out*/)
{
    int ret = 0;
    bool _esr = 0;

    *esr = 0;

    for (uint8_t i = 0; i <  number_endpoints; i++) {
        if (pepper_srv_endpoints[i].request_exposure) {
            ret = pepper_srv_endpoints[i].request_exposure(&_esr);
            if (ret) {
                ret |= (1 << i);
            }
            else {
                *esr |= (_esr << i);
            }
        }
    }

    if (*esr) {
        _set_status_led(CONFIG_PEPPER_SRV_ESR_LED, 0);
    }
    else {
        _set_status_led(CONFIG_PEPPER_SRV_ESR_LED, 1);
    }

    return ret;
}
