#include <stdio.h>
#include <stdlib.h>

#include "ztimer.h"

#include "shell.h"
#include "shell_commands.h"
#include "periph/rtc.h"
#include "event/thread.h"
#include "event/periodic.h"
#include "event/callback.h"

#include "twr.h"
#include "ebid.h"
#include "crypto_manager.h"
#include "uwb_epoch.h"
#include "desire_ble_scan.h"
#include "desire_ble_scan_params.h"
#include "desire_ble_adv.h"

#include "board.h"
#include "periph/gpio.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

#ifndef CONFIG_EPOCH_MARGIN_S
#define CONFIG_EPOCH_MARGIN_S       5
#endif

#ifndef CONFIG_TWR_EARLY_LISTEN
#define CONFIG_TWR_EARLY_LISTEN     3
#endif

#ifndef CONFIG_TWR_MIN_OFFSET
#define CONFIG_TWR_MIN_OFFSET       3
#endif

static uwb_epoch_data_t uwb_epoch_data;
static uwb_epoch_data_t uwb_epoch_data_serialize;
static uwb_ed_list_t uwb_ed_list;
static uwb_ed_memory_manager_t manager;
static crypto_manager_keys_t keys;
static ebid_t ebid;
static uint32_t start_time;
static event_periodic_t uwb_epoch_end;
static twr_event_mem_manager_t twr_manager;

#if defined(MODULE_PERIPH_GPIO_IRQ) && defined(BTN0_PIN)
static void _declare_positive(void *arg)
{
    (void) arg;
    LOG_INFO("[pepper]: COVID positive! \n");
    gpio_toggle(LED1_PIN);
}
#endif

uint16_t _get_txrx_offset(ebid_t *ebid)
{
    return (ebid->parts.ebid.u8[0] + (ebid->parts.ebid.u8[1] << 8)) %
           MS_PER_SEC + CONFIG_TWR_MIN_OFFSET;
}

/* this should be offloaded */
static void _adv_cb(uint32_t ts, void *arg)
{
    (void)arg;
    /* TODO:
        - might need some minimum offset to work properly
        - the adv callback might not be accurate enough
     */
    uwb_ed_t *next = (uwb_ed_t *)uwb_ed_list.list.next;
    if (!next) {
        LOG_DEBUG("[pepper]: no registered encounter\n");
    }
    do {
        next = (uwb_ed_t *)next->list_node.next;
        if (next->ebid.status.status == EBID_HAS_ALL) {
            if (next->seen_last_s + start_time + 5 > ts / MS_PER_SEC) {
                /* TODO: it should be the other way around as its commented out
                    but this seems to lead to worse synchronization... */
                // twr_schedule_request_managed((uint16_t) next->cid, _get_txrx_offset(&next->ebid));
                twr_schedule_listen_managed(_get_txrx_offset(&next->ebid));
            }
            else {
                LOG_DEBUG("[pepper]: skipping, not seen in a while");
            }
        }
    } while (next != (uwb_ed_t *)uwb_ed_list.list.next);
}

static void _detection_cb(uint32_t ts, const ble_addr_t *addr, int8_t rssi,
                          const desire_ble_adv_payload_t *adv_payload)
{
    (void)addr;
    (void)rssi;
    uint32_t cid;
    uint8_t sid;
    decode_sid_cid(adv_payload->data.sid_cid, &sid, &cid);
    LOG_DEBUG(
        "[pepper]: adv_data %" PRIx32 ": t=%" PRIu32 ", RSSI=%d, sid=%d \n",
        cid, ts, rssi, sid);
    /* process data */
    uwb_ed_t *uwb_ed = uwb_ed_list_process_slice(&uwb_ed_list, cid,
                                                 ts / MS_PER_SEC - start_time,
                                                 adv_payload->data.ebid_slice,
                                                 sid);
    if (uwb_ed->ebid.status.status == EBID_HAS_ALL) {
        /* TODO: it should be the other way around as its commented out
           but this seems to lead to worse synchronization... */
        // twr_schedule_listen_managed(_get_txrx_offset(&ebid) - CONFIG_TWR_EARLY_LISTEN);
        twr_schedule_request_managed((uint16_t)cid, _get_txrx_offset(&ebid));
    }
}

static void _twr_complete_cb(twr_event_data_t *data)
{
    LOG_INFO(
        "[pepper]: 0x%" PRIx16 " at %" PRIu16 "cm\n", data->addr, data->range);
    uwb_ed_list_process_rng_data(&uwb_ed_list, data->addr, data->time,
                                 data->range);
}

static void _boostrap_new_now(void)
{
    start_time = ztimer_now(ZTIMER_MSEC) / MS_PER_SEC;
    LOG_INFO("[pepper]: new uwb_epoch t=%" PRIu32 "\n", start_time);
    /* generate new keys */
    LOG_INFO("[pepper]: starting new uwb_epoch\n");
    /* reset uwb_epoch data */
    uwb_epoch_init(&uwb_epoch_data, ztimer_now(ZTIMER_EPOCH), &keys);
    /* update local ebid */
    ebid_init(&ebid);
    ebid_generate(&ebid, &keys);
    LOG_INFO("[pepper]: local ebid: [");
    for (uint8_t i = 0; i < EBID_SIZE; i++) {
        LOG_INFO("%d, ", ebid.parts.ebid.u8[i]);
    }
    LOG_INFO("]\n");
    /* start advertisement */
    LOG_INFO("[pepper]: start adv\n");
    desire_ble_adv_start(&ebid, CONFIG_SLICE_ROTATION_T_S,
                         CONFIG_EBID_ROTATION_T_S);
    /* set new short addr */
    twr_set_short_addr(desire_ble_adv_get_cid());
    /* start scanning */
    LOG_INFO("[pepper]: start scanning\n");
    desire_ble_scan_start(CONFIG_EBID_ROTATION_T_S * MS_PER_SEC);
}

static void _serialize_uwb_epoch_handler(event_t *event)
{
    (void)event;
    uwb_epoch_serialize_printf(&uwb_epoch_data_serialize);
}
static event_t _serialize_uwb_epoch =
{ .handler = _serialize_uwb_epoch_handler };

static void _end_of_uwb_epoch_handler(void *arg)
{
    (void)arg;
    LOG_INFO("[pepper]: end of uwb_epoch\n");
    /* stop advertising */
    desire_ble_adv_stop();
    /* process uwb_epoch data */
    LOG_INFO("[pepper]: process all uwb_epoch data\n");
    uwb_ed_list_finish(&uwb_ed_list);
    uwb_epoch_finish(&uwb_epoch_data, &uwb_ed_list);
    /* post uwb_epoch data process event, TODO: remove this extra variable*/
    memcpy(&uwb_epoch_data_serialize, &uwb_epoch_data,
           sizeof(uwb_epoch_data_t));
    event_post(EVENT_PRIO_MEDIUM, &_serialize_uwb_epoch);
    /* bootstrap new uwb_epoch */
    _boostrap_new_now();
}
static event_callback_t _end_of_uwb_epoch = EVENT_CALLBACK_INIT(
    _end_of_uwb_epoch_handler, NULL);

bool _time_is_in_range(uint32_t now, uint32_t new_now, uint32_t marge)
{
    if (now > new_now) {
        return (now - new_now) < marge;
    }
    else {
        return new_now - now < marge;
    }
}

void _time_update_cb(const current_time_ble_adv_payload_t *time)
{
    struct tm t;

    t.tm_year = time->data.year - 1900;
    t.tm_mon = time->data.month - 1;
    t.tm_mday = time->data.day;
    t.tm_hour = time->data.hours;
    t.tm_min = time->data.minutes;
    t.tm_sec = time->data.seconds;
    uint32_t new_now = rtc_mktime(&t);
    uint32_t now = ztimer_now(ZTIMER_EPOCH);
    LOG_DEBUG("[pepper]: epoch\n");
    LOG_DEBUG("\tcurrent:     %" PRIu32 "\n", now);
    LOG_DEBUG("\treceived:    %" PRIu32 "\n", new_now);
    /* adjust time only if out of CONFIG_EPOCH_MARGIN_S */
    bool adjust_time = !_time_is_in_range(now, new_now, CONFIG_EPOCH_MARGIN_S);
    /* if time change is too large reset epoch */
    bool reset_epoch = !_time_is_in_range(now, new_now,
                                          CONFIG_EPOCH_MARGIN_S / 10);
    if (adjust_time) {
        if (reset_epoch) {
            LOG_INFO("[pepper]: time was set back too much, bootstrap from 0\n");
            event_periodic_stop(&uwb_epoch_end);
            desire_ble_adv_stop();
            desire_ble_scan_stop();
        }
        ztimer_adjust_time(ZTIMER_EPOCH, new_now - now);
        LOG_DEBUG("\tnew-current: %" PRIu32 "\n", ztimer_now(ZTIMER_EPOCH));

        if (reset_epoch) {
            uint32_t modulo = ztimer_now(ZTIMER_EPOCH) %
                              CONFIG_EBID_ROTATION_T_S;
            uint32_t diff = modulo ? CONFIG_EBID_ROTATION_T_S - modulo : 0;
            /* setup end of uwb_epoch timeout event */
            event_periodic_init(&uwb_epoch_end, ZTIMER_EPOCH,
                                EVENT_PRIO_HIGHEST,
                                &_end_of_uwb_epoch.super);
            LOG_INFO(
                "[pepper]: delay for %" PRIu32 "s to align uwb_epoch start\n",
                diff);
            ztimer_sleep(ZTIMER_EPOCH, diff);
            event_periodic_start(&uwb_epoch_end, CONFIG_EBID_ROTATION_T_S);
            /* bootstrap new uwb_epoch */
            _boostrap_new_now();
        }
    }
}

static int _cmd_contact(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    gpio_toggle(LED0_PIN);
    LOG_INFO("[pepper]: testing needed!");
    return 0;
}

static const shell_command_t _commands[] = {
    {"contact", "Toggles led declaring contact => needs testing", _cmd_contact},
    {NULL, NULL, NULL}
};

int main(void)
{
#if defined(MODULE_PERIPH_GPIO_IRQ) && defined(BTN0_PIN)
    /* initialize a button to manually trigger an update */
    gpio_init_int(BTN0_PIN, BTN0_MODE, GPIO_FALLING, _declare_positive, NULL);
#endif

    /* initiate encounter management */
    uwb_ed_memory_manager_init(&manager);
    uwb_ed_list_init(&uwb_ed_list, &manager, &ebid);
    /* init twr */
    twr_event_mem_manager_init(&twr_manager);
    twr_managed_set_manager(&twr_manager);
    twr_init(EVENT_PRIO_HIGHEST);
    twr_register_rng_cb(_twr_complete_cb);
    twr_set_pan_id(0xaa);
    /* setup adv and scanning */
    desire_ble_adv_init();
    desire_ble_adv_set_cb(_adv_cb);
    desire_ble_scan_init(&desire_ble_scanner_params, _detection_cb);
    desire_ble_set_time_update_cb(_time_update_cb);
    /* setup end of uwb_epoch timeout event */
    uint32_t modulo = ztimer_now(ZTIMER_EPOCH) % CONFIG_EBID_ROTATION_T_S;
    uint32_t diff = modulo ? CONFIG_EBID_ROTATION_T_S - modulo : modulo;
    /* setup end of uwb_epoch timeout event */
    event_periodic_init(&uwb_epoch_end, ZTIMER_EPOCH, EVENT_PRIO_HIGHEST,
                        &_end_of_uwb_epoch.super);
    LOG_INFO("[pepper]: delay for %" PRIu32 "s to align uwb_epoch start\n",
             diff);
    ztimer_sleep(ZTIMER_EPOCH, diff);
    event_periodic_start(&uwb_epoch_end, CONFIG_EBID_ROTATION_T_S);
    /* bootstrap new uwb_epoch */
    _boostrap_new_now();
    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
