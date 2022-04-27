#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "periph/gpio.h"
#include "nimble_autoadv.h"
#include "event.h"
#include "event/thread.h"
#include "ztimer.h"

#include "pepper.h"
#include "pepper_srv.h"
#include "desire_ble_adv.h"

static ztimer_t debounce;

static void _debounce_cb(void *arg)
{
    (void)arg;
    gpio_irq_enable(BTN0_PIN);
}

static void _pepper_gatt_autoadv(event_t *event)
{
    (void)event;
    if (nimble_autoadv_is_active()) {
        puts("[pepper] gatt: stop");
        gpio_set(CONFIG_PEPPER_STATUS_LED);
        nimble_autoadv_stop();
    }
    else {
        puts("[pepper] gatt: start");
        gpio_clear(CONFIG_PEPPER_STATUS_LED);
        nimble_autoadv_start(NULL);
    }
}
static event_t _start_stop_event = { .handler = _pepper_gatt_autoadv };

static void _pepper_gatt_start_stop(void *arg)
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
    gpio_init_int(BTN0_PIN, BTN0_MODE, GPIO_FALLING, _pepper_gatt_start_stop, NULL);
    pepper_init();
    pepper_srv_init(EVENT_PRIO_MEDIUM);
    /* disable GATT server on start */
    nimble_autoadv_stop();
    /* TODO: could use main thread for something else */
    return 0;
}
