#include "pepper_srv.h"
#include "shell_commands.h"

XFA_USE_CONST(pepper_srv_endpoint_t, pepper_srv_endpoints);

static struct {
    bool exposed;
    bool infected;
    epoch_data_t* epoch_data; // TODO protect with a lock
} _coap_state = {false, false, NULL};


int _coap_srv_init(event_queue_t *evt_queue)
{
    evt_queue = evt_queue;
    printf(">--< End point CoAP : init !\n");

    return 0;
}


int _coap_srv_notify_epoch_data(epoch_data_t *epoch_data)
{
    epoch_data = epoch_data; // TODO protect with a lock
    
    printf(">--< End point CoAp : notify epoch_data, contacts = %d, ts = %ld\n", epoch_contacts(epoch_data), epoch_data->timestamp);
    
    return 0;
}

int _coap_srv_notify_infection(bool infected)
{
    _coap_state.infected = infected;
    // print infection data in json
    printf(">--< End point CoAP : infected ! %d\n", infected);
    
    return 0;
}


int _coap_srv_request_exposure(bool* esr /*out*/)
{
    *esr = _coap_state.exposed;
    
    return 0;
}


XFA_CONST(pepper_srv_endpoints, 0) pepper_srv_endpoint_t _pepper_srv_coap = {
    .init = _coap_srv_init,
    .notify_epoch_data = _coap_srv_notify_epoch_data,
    .notify_infection = _coap_srv_notify_infection,
    .request_exposure = _coap_srv_request_exposure
};
