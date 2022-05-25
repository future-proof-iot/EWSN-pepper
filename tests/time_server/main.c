/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       BLE Time Server
 *
 * @}
 */
#include "nimble_riot.h"
#include "host/ble_gap.h"

#include "net/bluetil/ad.h"

#include "event.h"
#include "event/callback.h"
#include "event/timeout.h"
#include "event/thread.h"
#include "random.h"
#include "ztimer.h"

#ifndef CONFIG_TIME_SERVER_ADV_INST
#define CONFIG_TIME_SERVER_ADV_INST         0
#endif
#define CONFIG_BLE_ADV_TX_POWER         127

/**
 * @brief The extended advertisement event duration
 *
 * This value should not matter, it will timeout because of the amount
 * of configured event
 */
#define ADV_DURATION_MS 1000

/* advertising data struct */
typedef void (*slotted_adv_cb_t)(bluetil_ad_t *ad, void *arg);

/**
 * @brief   Advertising managemer struct
 */
typedef struct {
    uint32_t itvl_ms;                           /**< the advertisement interval in ms */
    uint32_t advs;                              /**< current event count */
    uint32_t advs_max;                          /**< the total amount of advertisements */
    uint8_t instance;
    bluetil_ad_t *ad;
    slotted_adv_cb_t cb;
    void *arg;
} adv_config_t;

/**
 * @brief   Advertising event
 */
typedef struct {
    event_queue_t *queue;
    event_timeout_t timeout;    /**< the event timeout */
    event_callback_t event;     /**< the event callback */
    adv_config_t config;        /**< the advertsing manager */
} adv_event_t;

static void _configure_ext_adv(uint8_t instance, struct ble_gap_ext_adv_params *params)
{
    int rc;

    (void)rc;
    /* legacy PDUs do not support higher data rate */
    params->primary_phy = BLE_HCI_LE_PHY_1M;
    params->secondary_phy = BLE_HCI_LE_PHY_1M;
    /* TODO: check if this is the actual preffered tx power, not sure how
       it was handled before */
    params->tx_power = CONFIG_BLE_ADV_TX_POWER;
    /* sid is different from slice id using in desire_ble_pkt */
    params->sid = instance;
    /* use legacy PDUs */
    params->legacy_pdu = 1;
    /* TODO this section could be customizable */
    /* advertise using own */
    params->own_addr_type = nimble_riot_own_addr_type;

    rc = ble_gap_ext_adv_configure(instance, params, NULL, NULL, NULL);
    assert(rc == 0);
}

static void _advertise_once(uint8_t instance, uint8_t *bytes, size_t len)
{
    int rc;

    (void)rc;
    if (ble_gap_ext_adv_active(instance)) {
        rc = ble_gap_ext_adv_stop(instance);
        assert(rc == BLE_HS_EALREADY || rc == 0);
    }

    /* get mbuf for adv data, this is freed from the `ble_gap_ext_adv_set_data` */
    struct os_mbuf *data;

    data = os_msys_get_pkthdr(BLE_HS_ADV_MAX_SZ, 0);
    assert(data);

    /* fill mbuf with adv data */
    rc = os_mbuf_append(data, bytes, len);
    assert(rc == 0);
    /* set adv data */
    rc = ble_gap_ext_adv_set_data(instance, data);
    assert(rc == 0);

    /* set a single advertisement event */
    rc = ble_gap_ext_adv_start(instance, ADV_DURATION_MS / 10, 1);
    assert(rc == 0);
}

static void _slotted_adv_handler(void *arg)
{
    adv_event_t *adv_event = (adv_event_t *)arg;

    adv_event->config.advs++;

    /* re-arm timeout if needed, advs remaining or advs forever */
    if (adv_event->config.advs != adv_event->config.advs_max ||
        adv_event->config.advs_max == UINT32_MAX) {
        event_timeout_set(&adv_event->timeout, adv_event->config.itvl_ms);
    }

    /* callback to update advertising payload */
    if (adv_event->config.cb) {
        adv_event->config.cb(adv_event->config.ad, adv_event->config.arg);
    }

    /* advertise once */
    _advertise_once(adv_event->config.instance, adv_event->config.ad->buf,
                    adv_event->config.ad->pos);
}

void slotted_adv_init(adv_event_t *event, uint8_t instance, event_queue_t *queue,
                      struct ble_gap_ext_adv_params *params)
{
    event->queue = queue;
    event->config.instance = instance;

    /* init adv event */
    event_timeout_ztimer_init(&event->timeout, ZTIMER_MSEC, event->queue,
                              &event->event.super);
    event_callback_init(&event->event, _slotted_adv_handler, event);

    /* configure advertisement base parameters */
    _configure_ext_adv(instance, params);
}

void slotted_adv_stop(adv_event_t *event)
{
    if (ble_gap_ext_adv_active(event->config.instance)) {
        int rc = ble_gap_ext_adv_stop(event->config.instance);
        (void)rc;
        assert(rc == BLE_HS_EALREADY || rc == 0);
    }
    event_timeout_clear(&event->timeout);
}

void slotted_adv_start(adv_event_t *event, uint32_t itvl_ms, uint32_t advs_max,
                       bluetil_ad_t *ad, slotted_adv_cb_t cb, void *arg)
{
    /* stop ongoing advertisements if any */
    slotted_adv_stop(event);
    /* set advertisement parameters */
    event->config.advs = 0;
    event->config.advs_max = advs_max;
    event->config.itvl_ms = itvl_ms;
    event->config.cb = cb;
    event->config.arg = arg;
    event->config.ad = ad;
    /* either the callback to set the payload must not be NULL or a payload
       must be already set */
    assert(event->config.cb || event->config.ad->buf);
    /* setup first advertisement event */
    event_timeout_set(&event->timeout, event->config.itvl_ms);
}

// >>>>>>>>>>>>>>>>>>>>> Above this should be generic code
#include <stdio.h>
#include <stdlib.h>

#include "timex.h"
#include "periph/rtc.h"
#include "ztimer.h"
#include "shell.h"
#include "mutex.h"
#include "current_time.h"
#include "controller/ble_phy.h"

/* Advertising Event Thread spec */
#ifndef DEFAULT_ADV_ITVL_MS
#define DEFAULT_ADV_ITVL_MS     (10 * MS_PER_SEC)
#endif

/* buffer for ad */
static uint8_t buf[BLE_HS_ADV_MAX_SZ];
/* advertising data struct */
static bluetil_ad_t ad;
/* the advertisement event */
static adv_event_t adv_event;
/* the extended adv parameters */
static struct ble_gap_ext_adv_params params;
/* */
static mutex_t time_lock = MUTEX_INIT;

void set_epoch_adv_data(bluetil_ad_t *ad, void *arg)
{
    (void)arg;

    if (mutex_trylock(&time_lock)) {
        /* reset buffer */
        memset(ad->buf, 0, ad->size);
        ad->pos = 0;
        /* Tx power field added by the driver */
        int8_t phy_txpwr_dbm = ble_phy_txpwr_get();
        int rc = bluetil_ad_add(ad, BLE_GAP_AD_TX_POWER_LEVEL, &phy_txpwr_dbm,
                                sizeof(phy_txpwr_dbm));

        assert(rc == BLUETIL_AD_OK);
        /* Add service data uuid */
        uint16_t svc_uid = CURRENT_TIME_SERVICE_UUID16;

        rc = bluetil_ad_add(ad, BLE_GAP_AD_UUID16_COMP, &svc_uid, sizeof(svc_uid));
        assert(rc == BLUETIL_AD_OK);
        /* Add service data field */
        current_time_t current_time = { .epoch = ztimer_now(ZTIMER_EPOCH) };
        rc = bluetil_ad_add(ad, BLE_GAP_AD_SERVICE_DATA, &current_time.bytes, sizeof(current_time));
        assert(rc == BLUETIL_AD_OK);
        (void)rc;
        mutex_unlock(&time_lock);

        struct tm time;
        rtc_localtime(current_time.epoch, &time);
        printf("Current time:\n");
        printf("\tDate: %04d-%02d-%02d %02d:%02d:%02d,\n",
               time.tm_year + 1900,
               time.tm_mon + 1,
               time.tm_mday,
               time.tm_hour,
               time.tm_min,
               time.tm_sec);
        printf("\tEpoch: %"PRIu32"\n", current_time.epoch);
    }
}

static void _pre_cb(int32_t offset, void *arg)
{
    (void)arg;
    (void)offset;
    mutex_lock(&time_lock);
}
static current_time_hook_t _pre_hook = CURRENT_TIME_HOOK_INIT(_pre_cb, NULL);

static void _post_cb(int32_t offset, void *arg)
{
    (void)arg;
    (void)offset;
    mutex_unlock(&time_lock);
}
static current_time_hook_t _post_hook = CURRENT_TIME_HOOK_INIT(_post_cb, NULL);

int _cmd_adv_start(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    uint32_t itvl_ms = DEFAULT_ADV_ITVL_MS;

    if (argc == 2) {
        if (!strcmp(argv[1], "help")) {
            printf("usage: %s <advertisement period in seconds>\n", argv[0]);
            return 0;
        }
        itvl_ms = (uint32_t)atoi(argv[1]) * MS_PER_SEC;
    }
    slotted_adv_start(&adv_event, itvl_ms, UINT32_MAX, &ad, set_epoch_adv_data, NULL);
    return 0;
}

int _cmd_adv_stop(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("Stopped ongoing advertisements (if any)\n");
    slotted_adv_stop(&adv_event);
    return 0;
}

int _cmd_time(int argc, char** argv)
{
    if (argc == 1) {
        struct tm time;
        mutex_lock(&time_lock);
        uint32_t epoch = ztimer_now(ZTIMER_EPOCH);
        mutex_unlock(&time_lock);
        rtc_localtime(epoch, &time);
        printf("Current time:\n");
        printf("\tDate: %04d-%02d-%02d %02d:%02d:%02d,\n",
               time.tm_year + 1900,
               time.tm_mon + 1,
               time.tm_mday,
               time.tm_hour,
               time.tm_min,
               time.tm_sec);
        printf("\tEpoch: %"PRIu32"\n", epoch);
    }
    else if (argc == 2) {
        uint32_t epoch = (uint32_t) atoi(argv[1]);
        printf("new Epoch: %"PRIu32"\n", epoch);
        mutex_lock(&time_lock);
        ztimer_adjust_time(ZTIMER_EPOCH, epoch - ztimer_now(ZTIMER_EPOCH));
        mutex_unlock(&time_lock);
    }
    else if (argc == 7) {
        struct tm time;
        time.tm_year = (uint16_t)(atoi(argv[1])) - 1900;
        time.tm_mon = (uint8_t)(atoi(argv[2])) - 1;
        time.tm_mday = (uint8_t)(atoi(argv[3]));
        time.tm_hour = (uint8_t)(atoi(argv[4]));
        time.tm_min = (uint8_t)(atoi(argv[5]));
        time.tm_sec = (uint8_t)(atoi(argv[6]));
        mutex_lock(&time_lock);
        ztimer_adjust_time(ZTIMER_EPOCH, rtc_mktime(&time) - ztimer_now(ZTIMER_EPOCH));
        mutex_unlock(&time_lock);
    }
    else {
        puts("usage: time <year> <month=[1 = January, 12 = December]> <day> <hour> "
             "<min> <sec>");
        return -1;
    }
    return 0;
}

static const shell_command_t _commands[] = {
    { "start", "Starts Current Time advertisements", _cmd_adv_start },
    { "stop", "Stops Current Time advertisement", _cmd_adv_stop },
    { "time", "Sets or prints current RTC time", _cmd_time },
    { NULL, NULL, NULL }
};

int main(void)
{
    memset(&params, 0, sizeof(params));
    bluetil_ad_init(&ad, buf, 0, sizeof(buf));
    set_epoch_adv_data(&ad, NULL);
    slotted_adv_init(&adv_event, CONFIG_TIME_SERVER_ADV_INST, EVENT_PRIO_HIGHEST,
                     &params);

    /* current time setup */
    current_time_init();
    current_time_add_pre_cb(&_pre_hook);
    current_time_add_post_cb(&_post_hook);

    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
