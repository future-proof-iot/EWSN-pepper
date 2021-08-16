#include <stdio.h>
#include <stdlib.h>

#include "ztimer.h"

#include "shell.h"
#include "shell_commands.h"

#include "ebid.h"
#include "crypto_manager.h"
#include "epoch.h"
#include "event/thread.h"
#include "event/periodic.h"
#include "event/callback.h"
#include "desire_ble_scan.h"
#include "desire_ble_scan_params.h"
#include "desire_ble_adv.h"
#include "periph/rtc.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

static epoch_data_t epoch_data;
static epoch_data_t epoch_data_serialize;
static ed_list_t ed_list;
static ed_memory_manager_t manager;
static crypto_manager_keys_t keys;
static ebid_t ebid;
static uint32_t start_time;
static event_periodic_t epoch_end;

static void _detection_cb(uint32_t ts, const ble_addr_t *addr, int8_t rssi,
                          const desire_ble_adv_payload_t *adv_payload)
{
    uint32_t cid;
    uint8_t sid;

    decode_sid_cid(adv_payload->data.sid_cid, &sid, &cid);
    (void)addr;
    LOG_DEBUG("[desire]: adv_data %" PRIx32 ": t=%" PRIu32 ", RSSI=%d, sid=%d \n",
          cid, ts, rssi, sid);
    /* process data */
    ed_list_process_data(&ed_list, cid, ts / MS_PER_SEC - start_time,
                         adv_payload->data.ebid_slice, sid, (float)rssi);
}

static void _boostrap_new_epoch(void)
{
    start_time = ztimer_now(ZTIMER_EPOCH);
    LOG_INFO("[desire]: new epoch t=%" PRIu32 "\n", start_time);
    /* generate new keys */
    LOG_INFO("[desire]: starting new epoch\n");
    /* reset epoch data */
    epoch_init(&epoch_data, start_time, &keys);
    /* update local ebid */
    ebid_init(&ebid);
    ebid_generate(&ebid, &keys);
    LOG_INFO("[desire]: local ebid: [");
    for (uint8_t i = 0; i < EBID_SIZE; i++) {
        LOG_INFO("%d, ", ebid.parts.ebid.u8[i]);
    }
    LOG_INFO("]\n");
    /* start advertisement */
    LOG_INFO("[desire]: start adv\n");
    desire_ble_adv_start(&ebid, CONFIG_SLICE_ROTATION_T_S,
                         CONFIG_EBID_ROTATION_T_S);
    /* start scanning */
    LOG_INFO("[desire]: start scanning\n");
    desire_ble_scan_start(CONFIG_EBID_ROTATION_T_S * MS_PER_SEC);
}

static void _serialize_epoch_handler(event_t *event)
{
    (void)event;
    epoch_serialize_printf(&epoch_data_serialize);
}
static event_t _serialize_epoch = { .handler = _serialize_epoch_handler };

static void _end_of_epoch_handler(void *arg)
{
    (void)arg;
    LOG_INFO("[desire]: end of epoch\n");
    /* stop advertising */
    desire_ble_adv_stop();
    /* process epoch data */
    LOG_INFO("[desire]: process all epoch data\n");
    ed_list_finish(&ed_list);
    epoch_finish(&epoch_data, &ed_list);
    /* post epoch data process event, TODO: remove this extra variable*/
    memcpy(&epoch_data_serialize, &epoch_data, sizeof(epoch_data_t));
    event_post(EVENT_PRIO_MEDIUM, &_serialize_epoch);
    /* bootstrap new epoch */
    _boostrap_new_epoch();
}
static event_callback_t _end_of_epoch = EVENT_CALLBACK_INIT(
    _end_of_epoch_handler, NULL);

void _time_update_cb(const current_time_ble_adv_payload_t *time)
{
    struct tm t;

    t.tm_year = time->data.year - 1900;
    t.tm_mon = time->data.month - 1;
    t.tm_mday = time->data.day;
    t.tm_hour = time->data.hours;
    t.tm_min = time->data.minutes;
    t.tm_sec = time->data.seconds;
    uint32_t new_epoch = rtc_mktime(&t);
    uint32_t now = ztimer_now(ZTIMER_EPOCH);
    LOG_INFO("[desire]: received new time:\n");
    LOG_INFO("\tcurrent:  %" PRIu32 "\n", now);
    LOG_INFO("\treceived: %" PRIu32 "\n", new_epoch);
    if (now != new_epoch) {
        ztimer_adjust_time(ZTIMER_EPOCH, new_epoch - now);
        LOG_INFO("\tnew set:  %" PRIu32 "\n", ztimer_now(ZTIMER_EPOCH));
    }
    if (new_epoch + (CONFIG_EBID_ROTATION_T_S / 10) < now) {
        LOG_INFO("[desire]: time was set back too much, bootstrap from 0\n");
        /* if time was set back too much then the callback will take too
           long to execute */
        event_periodic_stop(&epoch_end);
        desire_ble_adv_stop();
        desire_ble_scan_stop();
        uint32_t modulo = ztimer_now(ZTIMER_EPOCH) % CONFIG_EBID_ROTATION_T_S;
        uint32_t diff = modulo ? CONFIG_EBID_ROTATION_T_S - modulo : 0;
        /* setup end of epoch timeout event */
        event_periodic_init(&epoch_end, ZTIMER_EPOCH, EVENT_PRIO_HIGHEST,
                            &_end_of_epoch.super);

        LOG_INFO("[desire]: delay for %" PRIu32 "s to align epoch start\n", diff);
        ztimer_sleep(ZTIMER_EPOCH, diff);
        event_periodic_start(&epoch_end, CONFIG_EBID_ROTATION_T_S);
        /* bootstrap new epoch */
        _boostrap_new_epoch();
    }
}

int main(void)
{
    /* initiate encounter management */
    ed_memory_manager_init(&manager);
    ed_list_init(&ed_list, &manager, &ebid);
    /* setup adv and scanning */
    desire_ble_adv_init_threaded();
    desire_ble_scan_init(&desire_ble_scanner_params, _detection_cb);
    desire_ble_set_time_update_cb(_time_update_cb);
    /* setup end of epoch timeout event */
    uint32_t modulo = ztimer_now(ZTIMER_EPOCH) % CONFIG_EBID_ROTATION_T_S;
    uint32_t diff = modulo ? CONFIG_EBID_ROTATION_T_S - modulo : modulo;
    /* setup end of epoch timeout event */
    event_periodic_init(&epoch_end, ZTIMER_EPOCH, EVENT_PRIO_HIGHEST,
                        &_end_of_epoch.super);
    LOG_INFO("[desire]: delay for %" PRIu32 "s to align epoch start\n", diff);
    ztimer_sleep(ZTIMER_EPOCH, diff);
    event_periodic_start(&epoch_end, CONFIG_EBID_ROTATION_T_S);
    /* bootstrap new epoch */
    _boostrap_new_epoch();
    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
