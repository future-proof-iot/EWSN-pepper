/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_epoch
 * @{
 *
 * @file
 * @brief       Random Epochs
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <string.h>
#include <assert.h>

#include "board.h"
#include "periph/gpio.h"

#include "ztimer.h"
#include "ztimer/periodic.h"
#include "memarray.h"

#include "ed.h"

#define ED_BLINK_ITVL        (100)

typedef struct {
    gpio_t pin;
    int count;
} toggle_count_t;

static toggle_count_t _toggle_count[4] = {
    {
        .pin = GPIO_UNDEF,
    },
    {
        .pin = GPIO_UNDEF,
    },
    {
        .pin = GPIO_UNDEF,
    },
    {
        .pin = GPIO_UNDEF,
    }
};
static ztimer_periodic_t _timer;

static int _blink(void *arg)
{
    (void)arg;

    for (uint8_t i = 0; i < 4; i++) {
        if (gpio_is_valid(_toggle_count[i].pin) && _toggle_count[i].count) {
            gpio_toggle(_toggle_count[i].pin);
            _toggle_count[i].count--;
        }
    }

    return 0;
}

static void _blink_init_start(void)
{
    if (!ztimer_is_set(ZTIMER_MSEC, &_timer.timer)) {
        ztimer_periodic_init(ZTIMER_MSEC, &_timer, _blink, NULL, ED_BLINK_ITVL);
        ztimer_periodic_start(&_timer);
    }
}

void ed_blink_start(gpio_t pin, uint32_t time_ms)
{
    for (uint8_t i = 0; i < 4; i++) {
        if (_toggle_count[i].pin == GPIO_UNDEF) {
            _toggle_count[i].pin = pin;
            _toggle_count[i].count = time_ms / ED_BLINK_ITVL;
            _blink_init_start();
            return;
        }
    }
}

void ed_blink_stop(gpio_t pin)
{
    bool stop = true;

    for (uint8_t i = 0; i < 4; i++) {
        if (_toggle_count[i].pin == pin) {
            _toggle_count[i].pin = GPIO_UNDEF;
        }
        else if (_toggle_count[i].pin != GPIO_UNDEF) {
            stop = false;
        }
    }
    if (stop) {
        ztimer_periodic_stop(&_timer);
    }
}
