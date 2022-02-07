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

#include "pepper.h"
#include "pepper_srv.h"
#include "storage.h"
#include "ztimer.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_DEBUG
#endif
#include "log.h"

#ifndef SDCARD_SPI_PARAM_CD
#define SDCARD_SPI_PARAM_CD                 GPIO_UNDEF
#endif

#ifndef PEPPER_SRV_SERIALIZE_BUFFER_SIZE
#define PEPPER_SRV_SERIALIZE_BUFFER_SIZE    2048
#endif

#define PEPPER_SRV_SD_CARD_PRESENT          (1 << 0)
#define PEPPER_SRV_SD_CARD_MOUNTED          (1 << 1)


XFA_USE_CONST(pepper_srv_endpoint_t, pepper_srv_endpoints);

/* internal status variable */
static uint8_t _status = 0;
/* debounce timer for card detecy singla */
static ztimer_t _cd_debouncer;
/* buffer for JSON or CBOR serialization of epoch_data */
static uint8_t _buffer[PEPPER_SRV_SERIALIZE_BUFFER_SIZE];
/* event queue */
static event_queue_t *_evt_queue = NULL;

static void _umount_sd_card(void *arg)
{
    (void)arg;
    if (_status & PEPPER_SRV_SD_CARD_MOUNTED) {
        if (storage_deinit() == 0) {
            LOG_DEBUG("[pepper_srv] storage: deinit storage\n");
            _status &= ~PEPPER_SRV_SD_CARD_MOUNTED;
        }
        else {
            LOG_DEBUG("[pepper_srv] storage: ERROR, failed to deinit storage\n");
        }
    }
}
static event_callback_t _umount_event = EVENT_CALLBACK_INIT(
    _umount_sd_card, NULL);

static void _debounce_cb(void *arg)
{
    (void)arg;
    gpio_irq_enable(SDCARD_SPI_PARAM_CD);
}

static void _card_detect(void *arg)
{
    (void)arg;
    ztimer_set(ZTIMER_MSEC, &_cd_debouncer, 1 * MS_PER_SEC);

    if (gpio_read(SDCARD_SPI_PARAM_CD) == 0) {
        LOG_DEBUG("[pepper_srv] storage: sdcard detected\n");
        _status |= PEPPER_SRV_SD_CARD_PRESENT;
    }
    else {
        LOG_DEBUG("[pepper_srv] storage: sdcard removed\n");
        _status &= ~PEPPER_SRV_SD_CARD_PRESENT;
        /* post unmounting event */
        event_post(_evt_queue, &_umount_event.super);
    }
}

int _storage_srv_init(event_queue_t *evt_queue)
{
    _evt_queue = evt_queue;
    if (gpio_is_valid(SDCARD_SPI_PARAM_CD)) {
        gpio_init_int(SDCARD_SPI_PARAM_CD, GPIO_IN_PU, GPIO_BOTH,
                      _card_detect, NULL);
        /* init software debounce timer */
        _cd_debouncer.callback = _debounce_cb;
    }
    if (storage_init() == 0) {
        _status |= PEPPER_SRV_SD_CARD_PRESENT;
        _status |= PEPPER_SRV_SD_CARD_MOUNTED;
    }

    return 0;
}

int _storage_srv_notify_epoch_data(epoch_data_t *epoch_data)
{
    LOG_DEBUG("[pepper_srv] storage: new data contacts = %d, ts = %ld\n",
              epoch_contacts(epoch_data), epoch_data->timestamp);
    if (_status & PEPPER_SRV_SD_CARD_PRESENT) {
        if (!(_status & PEPPER_SRV_SD_CARD_MOUNTED)) {
            /* mount sd card */
            LOG_DEBUG("[pepper_srv] storage: init storage\n");
            if (storage_init() == 0) {
                _status |= PEPPER_SRV_SD_CARD_MOUNTED;
            }
        }
    }

    LOG_DEBUG("[pepper_srv] storage:\n\tpresent=%s\n\tmounted=%s\n",
              (_status & PEPPER_SRV_SD_CARD_PRESENT) ? "true" : "false",
              (_status & PEPPER_SRV_SD_CARD_MOUNTED) ? "true" : "false");

    if (_status & PEPPER_SRV_SD_CARD_PRESENT &&
        _status & PEPPER_SRV_SD_CARD_MOUNTED) {
        /* serialize and store in sd-card */
        size_t len = contact_data_serialize_all_json(
            epoch_data, _buffer, sizeof(_buffer), pepper_get_serializer_bn()
            );
        if (storage_log(CONFIG_PEPPER_LOGFILE, _buffer, len - 1)) {
            LOG_DEBUG("[pepper_srv] storage: ERROR, failed to log\n");
        }
    }

    return 0;
}

XFA_CONST(pepper_srv_endpoints, 0) pepper_srv_endpoint_t _pepper_srv_storage = {
    .init = _storage_srv_init,
    .notify_epoch_data = _storage_srv_notify_epoch_data,
    .notify_infection = NULL,
    .request_exposure = NULL
};