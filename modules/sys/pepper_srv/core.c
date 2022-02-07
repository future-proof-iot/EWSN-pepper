#include "pepper_srv.h"
#include "event.h"

XFA_INIT_CONST(pepper_srv_endpoint_t, pepper_srv_endpoints);

static uint8_t number_endpoints = 0;
static event_queue_t * _evt_queue = NULL;

// init : init all endpoints and core
int pepper_srv_init(event_queue_t *evt_queue)
{
    int ret = 0;
    number_endpoints = (uint8_t) XFA_LEN(pepper_srv_endpoint_t, pepper_srv_endpoints);
    printf("pepper_srv_init : number_endpoints %d\n", number_endpoints);
    for (uint8_t i = 0; i <  number_endpoints ;i ++) {
        ret = pepper_srv_endpoints[i].init(evt_queue);
        if (ret) {
            ret |= (1 << i);
        }
    }
    _evt_queue = evt_queue; // keep for internal event handling

    return ret;
}

// notify (non-blocking) : add to ring buffer and notify all endpoints (post event handler for doing this)
int pepper_srv_notify_epoch_data(epoch_data_t* epoch_data)
{
    int ret = 0;

    // Blocking calls : TODO check if useful to do it in async way by posting event handler(s)
    for (uint8_t i = 0; i <  number_endpoints; i++) {
        ret = pepper_srv_endpoints[i].notify_epoch_data(epoch_data);
        if (ret) {
            ret |= (1 << i);
        }
    }
    return ret;
}

// Blocking call (?) : iterate on all endpoints for notifying them about infection
int pepper_srv_notify_infection(bool infected)
{
    int ret = 0;

    // Blocking calls : TODO check if useful to do it in async way by posting event handler(s)
    for (uint8_t i = 0; i <  number_endpoints; i++) {
        ret = pepper_srv_endpoints[i].notify_infection(infected);
        if (ret) {
            ret |= (1 << i);
        }
    }
    return ret;
}

// Blocking call (?) : iterate on all endpoints for quering infection, retrun 0 if none of endpoints returns true, otherwise a bitmap with an exposure flag per endpoint
int pepper_srv_esr (int* esr /*out*/) 
{
    int ret=0;
    bool _esr = 0;
    *esr = 0;

    for (uint8_t i = 0; i <  number_endpoints ;i ++) {
        ret = pepper_srv_endpoints[i].request_exposure(&_esr);
        if (ret) {
            ret |= (1 << i);
        } else {
            *esr |= (_esr << i);
        }
    }
    return ret;
}