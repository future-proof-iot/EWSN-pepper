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
 * @author      Anonymous
 * @author      Anonymous
 *
 * @}
 */

#include "xfa.h"
#include "event.h"
#include "event/callback.h"
#include "periph/gpio.h"

#include "pepper.h"
#include "pepper_srv.h"
#include "pepper_srv_storage.h"
#include "storage.h"
#include "ztimer.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
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

bool _storage_srv_sd_ready(void)
{
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
        return true;
    }
    return false;
}

int _storage_srv_notify_epoch_data(epoch_data_t *epoch_data)
{
    LOG_DEBUG("[pepper_srv] storage: new data contacts = %d, ts = %ld\n",
              epoch_contacts(epoch_data), epoch_data->timestamp);

    if (_storage_srv_sd_ready()) {
        /* serialize and store in sd_storage_srv_notify_epoch_data-card */
        size_t len = contact_data_serialize_all_json(
            epoch_data, _buffer, sizeof(_buffer), pepper_get_serializer_bn()
            );
        char logfile[CONFIG_PEPPER_BASE_NAME_BUFFER + sizeof(CONFIG_PEPPER_LOGS_DIR) +
                     sizeof(CONFIG_PEPPER_LOG_EXT)];
        // TODO: I wanted to use pepper_get_serializer_bn() but for some reason
        // it fails two often, need to investigate..
        sprintf(logfile, "%s%s%s", CONFIG_PEPPER_LOGS_DIR,
                CONFIG_PEPPER_SRV_STORAGE_EPOCH_FILE, CONFIG_PEPPER_LOG_EXT);
        if (storage_log(logfile, _buffer, len - 1)) {
            LOG_DEBUG("[pepper_srv] storage: ERROR, failed to log to %s\n", logfile);
            return -1;
        }
        LOG_DEBUG("[pepper_srv] storage: logged to %s\n", logfile);
    }

    return 0;
}

int _storage_srv_notify_uwb_data(ed_uwb_data_t *data)
{
    LOG_DEBUG("[pepper_srv] storage: new uwb data, ts = %ld\n",
              data->time);

    if (_storage_srv_sd_ready()) {
        /* serialize and store in sd_storage_srv_notify_epoch_data-card */
        size_t len = ed_serialize_uwb_json(data,
                                           pepper_get_serializer_bn(), _buffer, sizeof(_buffer));
        char logfile[CONFIG_PEPPER_BASE_NAME_BUFFER + sizeof(CONFIG_PEPPER_LOGS_DIR) +
                     sizeof(CONFIG_PEPPER_LOG_EXT)];
        // TODO: I wanted to use pepper_get_serializer_bn() but for some reason
        // it fails two often, need to investigate..
        sprintf(logfile, "%s%s%s", CONFIG_PEPPER_LOGS_DIR,
                CONFIG_PEPPER_SRV_STORAGE_UWB_DATA_FILE, CONFIG_PEPPER_LOG_EXT);
        if (storage_log(logfile, _buffer, len - 1)) {
            LOG_DEBUG("[pepper_srv] storage: ERROR, failed to log to %s\n", logfile);
            return -1;
        }
        LOG_DEBUG("[pepper_srv] storage: logged to %s\n", logfile);
    }

    return 0;
}


int _storage_srv_notify_ble_data(ed_ble_data_t *data)
{
    LOG_DEBUG("[pepper_srv] storage: new ble data, ts = %ld\n",
              data->time);

    if (_storage_srv_sd_ready()) {
        /* serialize and store in sd_storage_srv_notify_epoch_data-card */
        size_t len = ed_serialize_ble_json(data,
                                           pepper_get_serializer_bn(), _buffer, sizeof(_buffer));
        char logfile[CONFIG_PEPPER_BASE_NAME_BUFFER + sizeof(CONFIG_PEPPER_LOGS_DIR) +
                     sizeof(CONFIG_PEPPER_LOG_EXT)];
        // TODO: I wanted to use pepper_get_serializer_bn() but for some reason
        // it fails two often, need to investigate..
        sprintf(logfile, "%s%s%s", CONFIG_PEPPER_LOGS_DIR,
                CONFIG_PEPPER_SRV_STORAGE_BLE_DATA_FILE, CONFIG_PEPPER_LOG_EXT);
        if (storage_log(logfile, _buffer, len - 1)) {
            LOG_DEBUG("[pepper_srv] storage: ERROR, failed to log to %s\n", logfile);
            return -1;
        }
        LOG_DEBUG("[pepper_srv] storage: logged to %s\n", logfile);
    }

    return 0;
}


XFA_CONST(pepper_srv_endpoints, 0) pepper_srv_endpoint_t _pepper_srv_storage = {
    .init = _storage_srv_init,
    .notify_epoch_data = _storage_srv_notify_epoch_data,
    .notify_uwb_data = _storage_srv_notify_uwb_data,
    .notify_ble_data = _storage_srv_notify_ble_data,
    .notify_infection = NULL,
    .request_exposure = NULL
};
