#include "pepper_srv.h"
#include "event.h"
#include "event/callback.h"
#include "event/thread.h"
#include "mutex.h"
#include "xfa.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

XFA_INIT_CONST(pepper_srv_endpoint_t, pepper_srv_endpoints);

static uint8_t number_endpoints = 0;
static epoch_data_t _epoch_data;
static mutex_t _lock = MUTEX_INIT;
static event_queue_t *_evt_queue = NULL;

/* callback to notify new epoch_data */
static void _notify_epoch_data(void *arg)
{
    epoch_data_t* data = (epoch_data_t*)arg;
    pepper_srv_notify_epoch_data(data);
}
static event_callback_t _data_submit_event = EVENT_CALLBACK_INIT(
    _notify_epoch_data, &_epoch_data);

void pepper_srv_data_submit(epoch_data_t *data)
{
    /* try to acquire the lock, if failed, just drop the data */
    if (mutex_trylock(&_lock)) {
        memcpy(&_epoch_data, data, sizeof(epoch_data_t));
        event_post(_evt_queue, &_data_submit_event.super);
    }
    else {
        LOG_WARNING("[pepper_srv]: dropped epoch data\n");
    }
}

/* init : init all endpoints and core */
int pepper_srv_init(event_queue_t *evt_queue)
{
    int ret = 0;

    LOG_INFO("[pepper_srv]: init\n");
    number_endpoints = XFA_LEN(pepper_srv_endpoint_t, pepper_srv_endpoints);

    _evt_queue = evt_queue; /* keep for internal event handling */

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
    LOG_INFO("[pepper_srv]: number_endpoints %d\n", number_endpoints);
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
    mutex_unlock(&_lock);
    return ret;
}

/* Blocking call (?) : iterate on all endpoints for notifying them about infection */
int pepper_srv_notify_infection(bool infected)
{
    int ret = 0;

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
    return ret;
}
