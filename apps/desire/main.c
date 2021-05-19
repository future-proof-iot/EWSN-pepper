#include <stdio.h>
#include <stdlib.h>

#include "ztimer.h"

#include "shell.h"
#include "shell_commands.h"

#include "ebid.h"
#include "crypto_manager.h"
#include "epoch.h"
#include "event/thread.h"
#include "event/timeout.h"
#include "desire_ble_scan.h"
#include "desire_ble_adv.h"

#define ENABLE_DEBUG    1
#include "debug.h"

static epoch_data_t epoch_data;
static epoch_data_t epoch_data_serialize;
static ed_list_t ed_list;
static ed_memory_manager_t manager;
static crypto_manager_keys_t keys;
static ebid_t ebid;
static uint32_t start_time;
static event_timeout_t epoch_end;

void _dump_buffer(uint8_t *buf, size_t len, const char* prefix)
{
    if (prefix != NULL) {
        DEBUG("%s", prefix);
    }
    for (uint8_t i = 0; i < len; i++) {
        DEBUG("%02x ", buf[i]);
    }
    DEBUG_PUTS("");
}

static void _detection_cb(uint32_t ts, const ble_addr_t *addr, int8_t rssi,
                          const desire_ble_adv_payload_t *adv_payload)
{
    uint32_t cid;
    uint8_t sid;
    decode_sid_cid(adv_payload->data.sid_cid, &sid, &cid);
    (void) addr;
    DEBUG("[desire]: adv_data %"PRIx32": t=%"PRIu32", RSSI=%d, sid=%d \n",
           cid, ts, rssi, sid);
    /* process data */
    ed_list_process_data(&ed_list, cid, ts / MS_PER_SEC - start_time,
                         adv_payload->data.ebid_slice, sid, (float) rssi);
}

static void _boostrap_new_epoch(void)
{
    start_time = ztimer_now(ZTIMER_MSEC) / MS_PER_SEC;
    DEBUG("[desire]: new epoch t=%"PRIu32"\n", start_time);
    /* generate new keys */
    DEBUG_PUTS("[desire]: starting new epoch");
    /* reset epoch data */
    epoch_init(&epoch_data, start_time, &keys);
    /* update local ebid */
    ebid_init(&ebid);
    ebid_generate(&ebid, &keys);
    /* start advertisement */
    DEBUG_PUTS("[desire]: start adv");
    desire_ble_adv_start(&ebid, CONFIG_SLICE_ROTATION_T_S,
                         CONFIG_EBID_ROTATION_T_S);
    /* start scanning */
    DEBUG_PUTS("[desire]: start scanning");
    desire_ble_scan(CONFIG_EBID_ROTATION_T_S * MS_PER_SEC, _detection_cb);
    /* schedule next end of epoch */
    DEBUG_PUTS("[desire]: schedule end of epoch");
    event_timeout_set(&epoch_end, CONFIG_EBID_ROTATION_T_S * MS_PER_SEC);
}

static void _serialize_epoch_handler(event_t * event)
{
    (void) event;
    epoch_serialize_printf(&epoch_data_serialize);
}
static event_t _serialize_epoch = { .handler=_serialize_epoch_handler};

static void _end_of_epoch_handler(event_t * event)
{
    (void) event;
    DEBUG_PUTS("[desire]: end of epoch");
    /* stop advertising */
    desire_ble_adv_stop();
    /* process epoch data */
    DEBUG_PUTS("[desire]: process all epoch data");
    ed_list_finish(&ed_list);
    epoch_finish(&epoch_data, &ed_list);
    /* post epoch data process event, TODO: remove this extra variable*/
    memcpy(&epoch_data_serialize, &epoch_data, sizeof(epoch_data_t));
    event_post(EVENT_PRIO_MEDIUM, &_serialize_epoch);
    /* bootstrap new epoch */
    _boostrap_new_epoch();
}
static event_t _end_of_epoch = { .handler=_end_of_epoch_handler };

int main(void)
{
    /* initiate encounter management */
    ed_memory_manager_init(&manager);
    ed_list_init(&ed_list, &manager, &ebid);
    /* setup adv and scanning */
    desire_ble_adv_init();
    desire_ble_scan_init();
    /* setup end of epoch timeout event */
    event_timeout_ztimer_init(&epoch_end, ZTIMER_MSEC, EVENT_PRIO_HIGHEST,
                              &_end_of_epoch);
    /* bootstrap new epoch */
    _boostrap_new_epoch();

    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
