#include <stdio.h>
#include <stdlib.h>

#include "shell.h"
#include "shell_commands.h"

#include "time_ble_pkt.h"

#include "event.h"
#include "event/timeout.h"
#include "event/thread.h"

#include "assert.h"
#include "mutex.h"
#include "ztimer.h"
#include "timex.h"

#include "periph_conf.h"
#include "periph/rtc.h"

#include "nimble_autoadv.h"

#include "net/bluetil/ad.h"
#include "nimble/hci_common.h"

#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

/* BLE Advetisement specs  */
#define BLE_NIMBLE_ADV_DURATION_MS 10
current_time_ble_adv_payload_t adv_payload;
static void ble_advertise_once(current_time_ble_adv_payload_t *adv_payload);

/* Advertising Event Thread spec */
#ifndef DEFAULT_ADVERTISEMENT_PERIOD
#define DEFAULT_ADVERTISEMENT_PERIOD (1 * MS_PER_SEC)
#endif
static uint32_t _adv_period = DEFAULT_ADVERTISEMENT_PERIOD;
static event_queue_t _eq;
static event_t _update_evt;
static event_timeout_t _update_timeout_evt;
static void _tick_event_handler(event_t *e);
static char event_thread_stack[THREAD_STACKSIZE_MAIN];
static mutex_t _time_lock = MUTEX_INIT;

/* BLE Advertising Thread */
static bool adv_status = false;

static void cts_ble_adv_init(void)
{
    /* init event loop ticker thread
       create a thread that runs the event loop: event_thread_init */
    event_queue_init(&_eq);
    _update_evt.handler = _tick_event_handler;
    event_timeout_ztimer_init(&_update_timeout_evt, ZTIMER_MSEC, &_eq,
                              &_update_evt);

    /* Thread that will run an event loop (event_loop)
        for handling advertisment events */
    event_thread_init(&_eq, event_thread_stack, sizeof(event_thread_stack),
                      EVENT_QUEUE_PRIO_MEDIUM);

    adv_status = false;
}

static void cts_ble_adv_stop(void)
{
    /* notify stop event: cancel current advertisement ticker, reset */
    event_timeout_clear(&_update_timeout_evt);
    adv_status = false;
}

static void cts_ble_adv_start(uint32_t adv_period)
{
    /* stop current advetisement if any */
    if (adv_status) {
        cts_ble_adv_stop();
    }
    /* schedule advertisements */
    _adv_period = adv_period;
    event_timeout_set(&_update_timeout_evt, _adv_period);
    adv_status = true;
}

#define TM_YEAR_OFFSET      (1900)
static void print_time(const char *label, struct tm *time)
{
    uint32_t epoch = rtc_mktime(time);
    printf("%s  %04d-%02d-%02d %02d:%02d:%02d, epoch: %"PRIu32"\n", label,
           time->tm_year + TM_YEAR_OFFSET,
           time->tm_mon + 1,
           time->tm_mday,
           time->tm_hour,
           time->tm_min,
           time->tm_sec,
           epoch);
}

static void _tick_event_handler(event_t *e)
{
    struct tm time;

    (void)e;

    puts("[Tick]");
    /* get current time*/
    mutex_lock(&_time_lock);
    rtc_localtime(ztimer_now(ZTIMER_EPOCH), &time);
    mutex_unlock(&_time_lock);
    memset(adv_payload.bytes, 0, CURRENT_TIME_ADV_PAYLOAD_SIZE);
    adv_payload.data.service_uuid_16 = CURRENT_TIME_SERVICE_UUID16;
    adv_payload.data.year = (uint16_t)(time.tm_year + TM_YEAR_OFFSET);
    adv_payload.data.month = (uint8_t)(time.tm_mon + 1);
    adv_payload.data.day =  (uint8_t)(time.tm_mday);
    adv_payload.data.hours = (uint8_t)(time.tm_hour);
    adv_payload.data.minutes = (uint8_t)(time.tm_min);
    adv_payload.data.seconds = (uint8_t)(time.tm_sec);
    adv_payload.data.day_of_week = (uint8_t)(time.tm_wday);

    /* advertise it */
    print_time("Current Time =", &time);
    ble_advertise_once(&adv_payload);

    /* schedule next update event if advertisement duration not reached */
    event_timeout_set(&_update_timeout_evt, _adv_period);
}

/* BLE Advertismeent */
static struct  ble_gap_adv_params adv_params;
static void ble_advertise_once(current_time_ble_adv_payload_t *adv_payload)
{
    int nimble_ret;

    nimble_autoadv_init();

    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_NON;
    nimble_autoadv_set_ble_gap_adv_params(&adv_params);
    nimble_auto_adv_set_adv_duration(BLE_NIMBLE_ADV_DURATION_MS);

    /* Add service data uuid */
    uint16_t service_data = CURRENT_TIME_SERVICE_UUID16;

    nimble_ret = nimble_autoadv_add_field(BLE_GAP_AD_UUID16_COMP,
                                          &service_data, sizeof(service_data));
    assert(nimble_ret == BLUETIL_AD_OK);

    /* Add service data field */
    nimble_ret = nimble_autoadv_add_field(BLE_GAP_AD_SERVICE_DATA_UUID16,
                                          adv_payload->bytes,
                                          CURRENT_TIME_ADV_PAYLOAD_SIZE);
    assert(nimble_ret == BLUETIL_AD_OK);

    /* start, this will end after 10 ms approx */
    nimble_autoadv_start();

    (void)nimble_ret;
}

/* CLI handlers */
int _cmd_adv_start(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (((argc == 2) && (memcmp(argv[1], "help", 4) == 0))) {
        printf("usage: %s <advertisement eriod in seconds>\n",
               argv[0]);
        return 0;
    }

    uint32_t usr_adv_period = (argc == 1) ? DEFAULT_ADVERTISEMENT_PERIOD : (uint32_t)atoi(argv[1])*MS_PER_SEC;
    cts_ble_adv_start(usr_adv_period);

    return 0;
}

int _cmd_adv_stop(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    cts_ble_adv_stop();
    printf("Stopped ongoing advertisements (if any)\n");

    return 0;
}

int _cmd_time(int argc, char **argv)
{
    bool restart = adv_status;
    struct tm time;

    if (((argc == 2) && (memcmp(argv[1], "help", 4) == 0))) {
        printf("usage: %s <year> <month=[1 = January, 12 = December]> <day> <hour> <min> <sec>\n",
               argv[0]);
        return 0;
    }

    if (argc == 1) {
        mutex_lock(&_time_lock);
        rtc_localtime(ztimer_now(ZTIMER_EPOCH), &time);
        mutex_unlock(&_time_lock);
        print_time("Current Time =", &time);
        return 0;
    }

    if (argc != 7) {
        puts("Invalid call");
        return -1;
    }

    time.tm_year = (uint16_t)(atoi(argv[1])) - TM_YEAR_OFFSET;
    time.tm_mon = (uint8_t)(atoi(argv[2])) - 1;
    time.tm_mday = (uint8_t)(atoi(argv[3]));
    time.tm_hour = (uint8_t)(atoi(argv[4]));
    time.tm_min = (uint8_t)(atoi(argv[5]));
    time.tm_sec = (uint8_t)(atoi(argv[6]));

    /* if running stop, update rtc time, restart adv if previous */
    cts_ble_adv_stop();
    mutex_lock(&_time_lock);
    ztimer_adjust_time(ZTIMER_EPOCH, rtc_mktime(&time) - ztimer_now(ZTIMER_EPOCH));
    mutex_unlock(&_time_lock);

    if (restart) {
        puts("Time changed, restarting advertisement");
        cts_ble_adv_start(_adv_period);
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
    puts("Current RTC Time Broadcaster over BLE");

    /* initialize the static ebid and desire advertiser */
    cts_ble_adv_init();

    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
