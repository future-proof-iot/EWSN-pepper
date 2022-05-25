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
 * 2) Aperiodicall behaviour :
 *    - upon press on button : toggle infection status and red led. Notify serverimmediately.
 *
 * Purpose :
 *    - validate pepper_srv inteface behaviour
 *    - validate pepper_srv endpoints behaviour : shell, coap or storage
 */


#include <stdio.h>
#include <stdlib.h>

#include "net/sock/udp.h"

#include "pepper.h"
#include "pepper_srv.h"
#include "pepper_srv_coap.h"
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

#if IS_USED(MODULE_BLE_SCANNER_NETIF)
#include "ble_scanner.h"
#include "ble_scanner_params.h"
#include "ble_scanner_netif_params.h"
#endif

#ifndef CONFIG_PEPPER_SERVER_ADDR
#define CONFIG_PEPPER_SERVER_ADDR               "fd00:dead:beef::1"
#endif

/* dummy define to get rid of editor higliting */
#ifndef BOARD_NATIVE
#define BOARD_NATIVE    0
#endif

/* Epoch PoV of covid state */
struct {
    bool exposed;
    bool infected;
} _pepper_state = { false, false };

/* Event Loop Q qnd thread stack */
static event_periodic_t event_periodic;

/* epoch data pointer */
static epoch_data_t _epoch_data;

/* Button press handling : notify upon press, with 2 sec debouncing */
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
    event_post(EVENT_PRIO_MEDIUM, &_infected_event.super);
    debounce.callback = _debounce_cb;
    ztimer_set(ZTIMER_MSEC, &debounce, 2 * MS_PER_SEC);
}

/* Epoch event handling */
static void handle_epoch_end(event_t *event)
{
    (void)event;
    int ret = 0;
    bool esr = false;

    LOG_DEBUG("Tick EPOCH start: (exposed = %d, infected=%d)\n", _pepper_state.exposed,
              _pepper_state.infected);
    random_epoch(&_epoch_data);

    pepper_srv_data_submit(&_epoch_data);

    if (!(ret = pepper_srv_esr(&esr))) {
        if (esr) {
            LOG_INFO("[exposure_status]: COVID contact!\n");
        }
        else {
            LOG_INFO("[exposure_status]: No exposure!\n");
        }
    }
    else {
        LOG_ERROR("Internal error during esr : %d\n", ret);
    }

    LOG_DEBUG("Tick EPOCH end: (exposed = %d, infected=%d)\n", _pepper_state.exposed,
              _pepper_state.infected);
}
static event_t event_epoch_end = { .handler = handle_epoch_end };

int _cmd_coap_srv(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage: %s ip_addr port\n", argv[0]);
        return -1;
    }

    pepper_srv_coap_init_remote(argv[1], atoi(argv[2]));
    return 0;
}

static const shell_command_t _commands[] = {
    { "server", "get/sets the coap server host and port", _cmd_coap_srv },
    { NULL, NULL, NULL }
};

int main(void)
{
    puts("Pepper Server interface Test Application");

    /* initialize the uid module since the main pepper module is not used */
    pepper_uid_init();

#if IS_USED(MODULE_BLE_SCANNER_NETIF)
    /* initialize the desire scanner */
    ble_scanner_init(&ble_scan_params[1]);
    /* initializer netif scanner */
    ble_scanner_netif_init();
#endif

    if (BOARD_NATIVE) {
        ipv6_addr_t addr;
        const char addr_str[] = "2001:db8::2";
        ipv6_addr_from_str(&addr, addr_str);
        gnrc_netif_ipv6_addr_add(gnrc_netif_iter(NULL), &addr, 64, 0);
        pepper_srv_coap_init_remote("2001:db8::1", 5683);
    }

    /* gpio init */
    gpio_init_int(BTN0_PIN, BTN0_MODE, GPIO_FALLING, _handle_btn_press, NULL);

    /* Periodic event for epoch start */
    event_periodic_init(&event_periodic, ZTIMER_MSEC, EVENT_PRIO_MEDIUM, &event_epoch_end);

    /* Pepper server init¨*/
    pepper_srv_init(EVENT_PRIO_MEDIUM);

    event_periodic_start(&event_periodic, 10 * MS_PER_SEC);

    /* run shell on the main thread */
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
