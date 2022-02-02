#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "periph/gpio.h"
#include "pepper.h"
#include "desire_ble_adv.h"

#include "event.h"
#include "event/thread.h"
#include "msg.h"
#include "shell.h"
#include "shell_commands.h"
#include "ztimer.h"

#define MAIN_QUEUE_SIZE     (4)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

static ztimer_t debounce;

static void _debounce_cb(void *arg)
{
    (void)arg;
    gpio_irq_enable(BTN0_PIN);
}

static void _pepper_start_handler(event_t* event)
{
    (void)event;
    if (pepper_is_active()) {
        puts("[pepper] button: stop");
        pepper_stop();
    }
    else {
        pepper_start_params_t params = {
            .epoch_duration_s = CONFIG_EPOCH_DURATION_SEC,
            .epoch_iterations = 0,
            .adv_itvl_ms = CONFIG_BLE_ADV_ITVL_MS,
            .advs_per_slice = CONFIG_ADV_PER_SLICE,
            .scan_itvl_ms = CONFIG_BLE_SCAN_ITVL_MS,
            .scan_win_ms = CONFIG_BLE_SCAN_WIN_MS,
            .align = false,
        };
        puts("[pepper] button: start");
        pepper_start(&params);
    }
}
static event_t _start_stop_event = { .handler = _pepper_start_handler};

static void _pepper_start_stop(void *arg)
{
    (void)arg;
    gpio_irq_disable(BTN0_PIN);
    event_post(EVENT_PRIO_HIGHEST, &_start_stop_event);
    ztimer_set(ZTIMER_MSEC, &debounce, 2 * MS_PER_SEC);
}

int main(void)
{
    debounce.callback = _debounce_cb;

    /* initialize a button to stop pepper */
    gpio_init_int(BTN0_PIN, BTN0_MODE, GPIO_FALLING, _pepper_start_stop, NULL);
    pepper_init();
    /* the shell contains commands that receive packets via GNRC and thus
       needs a msg queue */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
