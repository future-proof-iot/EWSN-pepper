/*
 * Copyright (C) 2022 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_pepper_srv
 * @{
 *
 * @file
 * @brief       Pepper Server Interface implementation
 *
 * @author      Roudy Dagher <roudy.dagher@inria.fr>
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include "xfa.h"
#include "event.h"
#include "periph/gpio.h"

#include "pepper_srv.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_ERROR
#endif
#include "log.h"

XFA_USE_CONST(pepper_srv_t, pepper_srvs);

static void cd(void *arg)
{
    (void)arg;
    uint16_t status = pepper_srv_get_status();
    pepper_srv_set_status(status ^ PEPPER_STATUS_SD_CARD_PRESENT);
    pepper_srv_state_change();
    /* set timer to disable callback to debounce */
}

void _storage_srv_init(void)
{
    if (gpio_init_int(SDCARD_SPI_PARAM_CD, GPIO_IN_PU, GPIO_BOTH, cd, (void *)cnt) < 0) {
        LOG_DEBUG("[FAILED] init BTN3!");
        return 1;
    }
}

void _storage_data_rdy(event_t *event)
{
    if (status & PEPPER_STATUS_SD_CARD_PRESENT &&
        status & PEPPER_SD_CARD_MOUNTED) {
            /* serialize and store in sd-card */
        }
    }
}

void _storage_state_change_event(event_t *event)
{
    if (status & PEPPER_STATUS_SD_CARD_PRESENT) {
        if (status & ~PEPPER_SD_CARD_MOUNTED) {
            /* mount sd card */
        }
    }
    else {
        if (status & PEPPER_SD_CARD_MOUNTED) {
            /* unmount sd card since there is no longer an sd card */
        }
    }
}

XFA_CONST(pepper_srvs, 0) pepper_srv_t _pepper_srv_storage = {
    .driver = {
        .init = _storage_srv_init,
        .notify_epoch_data = _storage_srv_event,
    }
}
