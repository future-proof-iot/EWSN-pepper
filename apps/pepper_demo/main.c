#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "periph/gpio.h"

#include "event.h"
#include "event/thread.h"
#include "event/periodic.h"
#include "msg.h"
#include "shell.h"
#include "shell_commands.h"
#include "ztimer.h"
#include "timex.h"

#include "pepper.h"
#include "pepper_srv.h"
#include "desire_ble_adv.h"

#ifndef CONFIG_PEPPER_DEMO_ESR_PERIOD_MS
#define CONFIG_PEPPER_DEMO_ESR_PERIOD_MS    (60 * MS_PER_SEC)
#endif

#define MAIN_QUEUE_SIZE     (4)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

static ztimer_t debounce;
struct {
    bool exposed;
    bool infected;
} _state = { false, false };

static void _esr_handler(event_t *event);
static event_periodic_t _periodic_esr;
static event_t _event_esr = { .handler = _esr_handler };

/* Button press handling : notify upon press, with 2 sec debouncing */
static void _debounce_cb(void *arg)
{
    (void)arg;
    gpio_irq_enable(BTN0_PIN);
}

static void _update_infected_status(void *arg)
{
    (void)arg;
    pepper_srv_notify_infection(_state.infected);
    LED1_TOGGLE;
    if (_state.infected) {
        printf("[infected_declaration]: COVID positive!\n");
    }
}
static event_callback_t _infected_event = EVENT_CALLBACK_INIT(
    _update_infected_status, NULL);

static void _toggle_infected(void *arg)
{
    (void)arg;
    gpio_irq_disable(BTN0_PIN);
    _state.infected ^= true;
    event_post(EVENT_PRIO_LOWEST, &_infected_event.super);
    debounce.callback = _debounce_cb;
    ztimer_set(ZTIMER_MSEC, &debounce, 2 * MS_PER_SEC);
}

static void _esr_handler(event_t *event)
{
    (void)event;
    if (pepper_srv_esr(&_state.exposed) == 0) {
        if (_state.exposed) {
            printf("[exposure_status]: COVID contact!\n");
        }
        else {
            printf("[exposure_status]: No exposure!\n");
        }
    }
}

int main(void)
{
    debounce.callback = _debounce_cb;

    /* initializer netif scanner */
    ble_scanner_netif_init();

    /* initialize a button to stop pepper */
    gpio_init_int(BTN0_PIN, BTN0_MODE, GPIO_FALLING, _toggle_infected, NULL);
    pepper_init();
    pepper_srv_init(EVENT_PRIO_LOWEST);

    /* periodic event for epoch start */
    event_periodic_init(&_periodic_esr, ZTIMER_MSEC, EVENT_PRIO_LOWEST, &_event_esr);
    event_periodic_start(&_periodic_esr, CONFIG_PEPPER_DEMO_ESR_PERIOD_MS);

    /* start pepper with default parameters */
    pepper_start_params_t params = {
        .epoch_duration_s = CONFIG_EPOCH_DURATION_SEC,
        .epoch_iterations = 0,
        .adv_itvl_ms = CONFIG_BLE_ADV_ITVL_MS,
        .advs_per_slice = CONFIG_ADV_PER_SLICE,
        .scan_itvl_ms = CONFIG_BLE_SCAN_ITVL_MS,
        .scan_win_ms = CONFIG_BLE_SCAN_WIN_MS,
        .align = false,
    };

    puts("Pepper Demo Applicaiton, start with default parameters");
    pepper_start(&params);

    /* the shell contains commands that receive packets via GNRC and thus
       needs a msg queue */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
