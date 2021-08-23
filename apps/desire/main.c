#include <stdio.h>
#include <stdlib.h>

#include "ztimer.h"
#include "shell.h"
#include "shell_commands.h"
#include "event/periodic.h"
#include "event/thread.h"
#include "event/callback.h"

#include "ebid.h"
#include "crypto_manager.h"
#include "epoch.h"
#include "desire_ble_scan.h"
#include "desire_ble_scan_params.h"
#include "desire_ble_adv.h"
#include "current_time.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

#ifndef CONFIG_EPOCH_MAX_TIME_OFFSET
#define CONFIG_EPOCH_MAX_TIME_OFFSET            (CONFIG_EBID_ROTATION_T_S / 10)
#endif

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
    start_time = ztimer_now(ZTIMER_MSEC) / MS_PER_SEC;
    LOG_INFO("[desire]: new epoch t=%" PRIu32 "\n",
             (uint32_t)ztimer_now(ZTIMER_EPOCH));
    /* generate new keys */
    LOG_INFO("[desire]: starting new epoch\n");
    /* reset epoch data */
    epoch_init(&epoch_data, ztimer_now(ZTIMER_EPOCH), &keys);
    /* update local ebid */
    ebid_init(&ebid);
    LOG_INFO("[desire]: new ebid generation\n");
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
    LOG_INFO("[desire]: dumping epoch data\n");
    epoch_serialize_printf(&epoch_data_serialize);
}
static event_t _serialize_epoch = { .handler = _serialize_epoch_handler };

static void _end_of_epoch_handler(void *arg)
{
    (void)arg;
    LOG_INFO("[desire]: end of epoch\n");
    /* stop advertising */
    desire_ble_adv_stop();
    desire_ble_scan_stop();
    /* process epoch data */
    LOG_INFO("[desire]: process all epoch data\n");
    ed_list_finish(&ed_list);
    epoch_finish(&epoch_data, &ed_list);
    /* post epoch data process event, TODO: remove this extra variable*/
    memcpy(&epoch_data_serialize, &epoch_data, sizeof(epoch_data_t));
    /* bootstrap new epoch */
    _boostrap_new_epoch();
    /* post after bootstrapping the new epoch */
    event_post(EVENT_PRIO_MEDIUM, &_serialize_epoch);
}
static event_callback_t _end_of_epoch = EVENT_CALLBACK_INIT(
    _end_of_epoch_handler, NULL);

static void _aligned_epoch_start(void)
{
    /* setup end of epoch timeout event */
    uint32_t modulo = ztimer_now(ZTIMER_EPOCH) % CONFIG_EBID_ROTATION_T_S;
    uint32_t diff = modulo ? CONFIG_EBID_ROTATION_T_S - modulo : 0;

    /* setup end of epoch timeout event */
    event_periodic_init(&epoch_end, ZTIMER_EPOCH, EVENT_PRIO_HIGHEST,
                        &_end_of_epoch.super);
    LOG_INFO("[pepper]: delay for %" PRIu32 "s to align uwb_epoch start\n",
             diff);
    ztimer_sleep(ZTIMER_EPOCH, diff);
    event_periodic_start(&epoch_end, CONFIG_EBID_ROTATION_T_S);
    /* bootstrap new epoch */
    _boostrap_new_epoch();
}

static bool _epoch_reset = false;
static void _pre_adjust_time(int32_t offset, void *arg)
{
    (void)arg;
    _epoch_reset = offset > 0 ? offset > (int32_t) CONFIG_EPOCH_MAX_TIME_OFFSET :
                   -offset > (int32_t) CONFIG_EPOCH_MAX_TIME_OFFSET;
    if (_epoch_reset) {
        LOG_INFO("[pepper]: time was set back too much, bootstrap from 0\n");
        event_periodic_stop(&epoch_end);
        desire_ble_adv_stop();
        desire_ble_scan_stop();
    }
}
static current_time_hook_t _pre_hook;
static void _post_adjust_time(int32_t offset, void *arg)
{
    (void)offset;
    bool *epoch_restart = (bool *)arg;
    if (*epoch_restart) {
        _aligned_epoch_start();
        *epoch_restart = false;
    }
}
static current_time_hook_t _post_hook;

int main(void)
{
    /* initiate encounter management */
    ed_memory_manager_init(&manager);
    ed_list_init(&ed_list, &manager, &ebid);
    /* setup adv and scanning */
    desire_ble_adv_init(EVENT_PRIO_HIGHEST);
    desire_ble_scan_init(&desire_ble_scanner_params, _detection_cb);
    /* init ble scanner and current_time */
    current_time_init();
    current_time_hook_init(&_pre_hook, _pre_adjust_time, &_epoch_reset);
    current_time_hook_init(&_post_hook, _post_adjust_time, &_epoch_reset);
    current_time_add_pre_cb(&_pre_hook);
    current_time_add_post_cb(&_post_hook);
    /* begin and align epoch */
    _aligned_epoch_start();
    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
