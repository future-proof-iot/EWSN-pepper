#include <stdio.h>
#include <stdlib.h>

#include "ztimer.h"

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
static ed_list_t ed_list;
static ed_memory_manager_t manager;
static crypto_manager_keys_t keys;
static ebid_t ebid;
static uint32_t start_time;
static event_timeout_t epoch_end;

static void _detection_cb(uint32_t ts, const ble_addr_t *addr, int8_t rssi,
                  const desire_ble_adv_payload_t *adv_payload)
{
    uint32_t cid;
    uint8_t sid;
    decode_sid_cid(adv_payload->data.sid_cid, &sid, &cid);
    (void) addr;
    DEBUG("[desire]: adv_data %"PRIu32": t=%"PRIu32", RSSI=%d, sid=%d \n",
           cid, ts, rssi, sid);
    /* process data */
    ed_list_process_data(&ed_list, cid, ts / MS_PER_SEC - start_time, adv_payload->data.ebid_slice,
                         sid, (float) rssi);
}

static void _boostrap_new_epoch(void)
{
    start_time = ztimer_now(ZTIMER_MSEC) / MS_PER_SEC;
    DEBUG("[desire]: new epoch t=%"PRIu32"\n", start_time);
    /* generate new keys */
    DEBUG_PUTS("[desire]: generating new keys");
    crypto_manager_gen_keypair(&keys);
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
    //epoch_serialize_printf(&epoch_data);
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
    epoch_finish(&epoch_data, &ed_list, &keys);
    epoch_serialize_printf(&epoch_data);
    /* post epoch data process event */
    event_post(EVENT_PRIO_MEDIUM, &_serialize_epoch);
    /* bootstrap new epoch */
    _boostrap_new_epoch();
}
static event_t _end_of_epoch = { .handler=_end_of_epoch_handler };

int main(void)
{
    /* initiate encounter management */
    ed_memory_manager_init(&manager);
    ed_list_init(&ed_list, &manager);
    /* setup adv and scanning */
    desire_ble_adv_init();
    desire_ble_scan_init(EVENT_PRIO_MEDIUM);
    /* setup end of epoch timeout event */
    event_timeout_ztimer_init(&epoch_end, ZTIMER_MSEC, EVENT_PRIO_HIGHEST,
                              &_end_of_epoch);
    /* bootstrap new epoch */
    _boostrap_new_epoch();

    return 0;
}
