/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/*
 * This test app emulates a pepper application that performs the following :
 * 1) Periodical epoch behaviour :
 *    - generate random epoch data and notify server
 *    - query exposure from server : blue led is on if exposed, off otherwise
 * 2) Aperiodical bhaviour :
 *    - upon press on button : toogle infection status and red led. Notify serverimmediately.
 *
 * Purpose :
 *    - validate pepper_srv inteface behaviour
 *    - validate pepper_srv endpoints behaviour : shell, coap or storage
 */


#include <stdio.h>
#include <stdlib.h>

#include "pepper_srv.h"
#include "epoch.h"

#include "event/thread.h"
#include "event/periodic.h"
#include "event/callback.h"

#include "shell_commands.h"

#include "board.h"
#include "periph/gpio.h"
#include "ztimer.h"
#include "timex.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_ERROR
#endif
#include "log.h"

/* Epoch PoV of covid state */
struct {
    bool exposed;
    bool infected;
} _pepper_state = { false, false };

// Button press handling : notify upon press, with 2 sec debouncing
static void _debounce_cb(void *arg)
{
    (void)arg;
    gpio_irq_enable(BTN0_PIN);
}

static void _update_infected_status(void *arg)
{
    (void)arg;
    pepper_srv_notify_infection(_pepper_state.infected); // immediate notification
}
static event_callback_t _infected_event = EVENT_CALLBACK_INIT(
    _update_infected_status, NULL);

static void _handle_btn_press(void *arg)
{
    (void)arg;
    static ztimer_t debounce;

    gpio_irq_disable(BTN0_PIN);
    _pepper_state.infected = !_pepper_state.infected;
    LED1_TOGGLE;
    event_post(EVENT_PRIO_MEDIUM, &_infected_event.super);
    debounce.callback = _debounce_cb;
    ztimer_set(ZTIMER_MSEC, &debounce, 2 * MS_PER_SEC);
}


// Epoch event handling
static epoch_data_t _epoch_data;
static void handle_epoch_end(event_t *event)
{
    int ret, esr = 0;

    LOG_DEBUG("Tick EPOCH start: (exposed = %d, infected=%d)\n", _pepper_state.exposed,
           _pepper_state.infected);
    random_epoch(&_epoch_data);

    ret = pepper_srv_notify_epoch_data(&_epoch_data);
    if (ret) {
        LOG_ERROR("Internal error during notif epoch data : %d", ret);
    }

    if (!(ret = pepper_srv_esr(&esr))) {
        if (esr) {
            LOG_INFO("[exposure_status]: COVID contact!\n");
            LED3_ON;
        }
        else {
            LOG_INFO("[exposure_status]: No exposure!\n");
            LED3_OFF;
        }
    }
    else {
        LOG_ERROR("Internal error during esr : %d", ret);
    }

    LOG_DEBUG("Tick EPOCH end: (exposed = %d, infected=%d)\n", _pepper_state.exposed,
           _pepper_state.infected);
    event = event;
}
static event_t event_epoch_end = { .handler = handle_epoch_end };


/* Event Loop Q qnd thread stack */
static event_queue_t evt_queue;
static char _stack[THREAD_STACKSIZE_DEFAULT];
static event_periodic_t event_periodic;

int main(void)
{
    puts("Pepper Server interface Test Application");

    /* gpio init */
    gpio_init_int(BTN0_PIN, BTN0_MODE, GPIO_FALLING, _handle_btn_press, NULL);
    LED1_OFF;
    LED3_OFF;

    /* Event loop thread init */
    event_queue_init(&evt_queue);
    event_thread_init(&evt_queue, _stack, sizeof(_stack),
                      THREAD_PRIORITY_MAIN - 1);


    /* Periodic event for epoch start */
    event_periodic_init(&event_periodic, ZTIMER_MSEC, &evt_queue, &event_epoch_end);


    /* Pepper server init¨*/
    pepper_srv_init(&evt_queue);

    event_periodic_start(&event_periodic, 5 * MS_PER_SEC);

    /* run shell on the main thread */
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
