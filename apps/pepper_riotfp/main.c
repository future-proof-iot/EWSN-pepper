#include <stdio.h>
#include <stdlib.h>

#include "ztimer.h"
#include "random.h"
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
#include "current_time.h"

#include "suit/transport/coap.h"

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
#include "coap/utils.h"
#endif
#include "state_manager.h"
#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

#include "board.h"
#include "periph/gpio.h"
#ifndef CONFIG_EPOCH_SILENT_PERIOD_MIN_S
#define CONFIG_EPOCH_SILENT_PERIOD_MIN_S        1
#endif

#ifndef CONFIG_EPOCH_SILENT_PERIOD_MAX_S
#define CONFIG_EPOCH_SILENT_PERIOD_MAX_S        5
#endif

#ifndef CONFIG_EPOCH_MAX_TIME_OFFSET
#define CONFIG_EPOCH_MAX_TIME_OFFSET            (CONFIG_EBID_ROTATION_T_S / 10)
#endif

#ifndef CONFIG_TWR_EARLY_LISTEN
#define CONFIG_TWR_EARLY_LISTEN                 3
#endif

#ifndef CONFIG_TWR_MIN_OFFSET
#define CONFIG_TWR_MIN_OFFSET                   3
#endif

#ifndef CONFIG_PEPPER_SERVER_PORT
#define CONFIG_PEPPER_SERVER_PORT               5683
#endif

#ifndef CONFIG_PEPPER_SERVER_ADDR
#define CONFIG_PEPPER_SERVER_ADDR               "fd00:dead:beef::1"
#endif

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
static sock_udp_ep_t remote;
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
static void _update_infected_status(void *arg)
{
    (void)arg;
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
    if (desire_ble_is_connected()) {
        state_manager_coap_send_infected();
    }
#endif
}
static event_callback_t _infected_event = EVENT_CALLBACK_INIT(
    _update_infected_status, NULL);
static void _declare_positive(void *arg)
{
    (void)arg;
    state_manager_set_infected_status(!state_manager_get_infected_status());
    event_post(EVENT_PRIO_MEDIUM, &_infected_event.super);
}
#endif

uint16_t _get_txrx_offset(ebid_t *ebid)
{
    return (ebid->parts.ebid.u8[0] + (ebid->parts.ebid.u8[1] << 8)) %
           MS_PER_SEC + CONFIG_TWR_MIN_OFFSET;
}

/* TODO: this should be offloaded */
static void _adv_cb(uint32_t ts, void *arg)
{
    (void)arg;
    /* TODO:
        - might need some minimum offset to work properly
        - the adv callback might not be accurate enough
     */
    uwb_ed_t *next = (uwb_ed_t *)uwb_ed_list.list.next;
    if (!next) {
        LOG_INFO("[pepper]: skip, no neigh\n");
        return;
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
                LOG_DEBUG("[pepper]: skip, MIA\n");
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

/* Event to allow delaying adv/scan activity start */
static void _scan_adv_start(void *arg)
{
    (void)arg;
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
static event_timeout_t _silent_timeout;
static event_callback_t _scan_adv_start_ev = EVENT_CALLBACK_INIT(
    _scan_adv_start, NULL);

static void _boostrap_new_epoch(void)
{
    start_time = ztimer_now(ZTIMER_MSEC) / MS_PER_SEC;
    LOG_INFO("[pepper]: new uwb_epoch t=%" PRIu32 "\n", (uint32_t) ztimer_now(ZTIMER_EPOCH));
    /* initiate epoch and generate new keys */
    uwb_epoch_init(&uwb_epoch_data, ztimer_now(ZTIMER_EPOCH), &keys);
    /* update local ebid */
    ebid_init(&ebid);
    LOG_INFO("[pepper]: new ebid generation\n");
    ebid_generate(&ebid, &keys);
    LOG_INFO("[pepper]: local ebid: [");
    for (uint8_t i = 0; i < EBID_SIZE; i++) {
        LOG_INFO("%d, ", ebid.parts.ebid.u8[i]);
    }
    LOG_INFO("]\n");
    /* random silent period */
    event_timeout_set(&_silent_timeout,
                      random_uint32_range(CONFIG_EPOCH_SILENT_PERIOD_MIN_S,
                                          CONFIG_EPOCH_SILENT_PERIOD_MAX_S));
}

typedef struct {
    event_t super;
    uwb_epoch_data_t *data;
} uwb_epoch_data_event_t;

static void _serialize_uwb_epoch_handler(event_t *event)
{
    uwb_epoch_data_event_t *d_event = (uwb_epoch_data_event_t *)event;

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
    if (desire_ble_is_connected()) {
        if (uwb_epoch_contacts(d_event->data)) {
            state_manager_coap_send_ertl(d_event->data);
        }
    }
    else {
#endif
    LOG_INFO("[pepper]: not connected, dumping epoch data\n");
    uwb_epoch_serialize_printf(d_event->data);
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
}
#endif
}

static uwb_epoch_data_event_t _serialize_uwb_epoch =
{ .super.handler = _serialize_uwb_epoch_handler };

static void _end_of_uwb_epoch_handler(void *arg)
{
    (void)arg;
    LOG_INFO("[pepper]: end of uwb_epoch\n");
    /* stop advertising and scanning */
    desire_ble_scan_stop();
    desire_ble_adv_stop();
    /* process uwb_epoch data */
    LOG_INFO("[pepper]: process all uwb_epoch data\n");
    uwb_ed_list_finish(&uwb_ed_list);
    uwb_epoch_finish(&uwb_epoch_data, &uwb_ed_list);
    /* post serializing/offloading event */
    memcpy(&uwb_epoch_data_serialize, &uwb_epoch_data,
           sizeof(uwb_epoch_data_t));
    _serialize_uwb_epoch.data = &uwb_epoch_data_serialize;
    /* bootstrap new uwb_epoch */
    _boostrap_new_epoch();
    /* post after bootstrapping the new epoch */
    event_post(EVENT_PRIO_MEDIUM, &_serialize_uwb_epoch.super);
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
    /* if connected then update exposure status */
    if (desire_ble_is_connected()) {
        state_manager_coap_get_esr();
    }
#endif
}

static event_callback_t _end_of_uwb_epoch = EVENT_CALLBACK_INIT(
    _end_of_uwb_epoch_handler, NULL);

static void _aligned_epoch_start(void)
{
    /* setup end of uwb_epoch timeout event */
    uint32_t modulo = ztimer_now(ZTIMER_EPOCH) % CONFIG_EBID_ROTATION_T_S;
    uint32_t diff = modulo ? CONFIG_EBID_ROTATION_T_S - modulo : 0;

    /* setup end of uwb_epoch timeout event */
    event_periodic_init(&uwb_epoch_end, ZTIMER_EPOCH, EVENT_PRIO_HIGHEST,
                        &_end_of_uwb_epoch.super);
    LOG_INFO("[pepper]: delay for %" PRIu32 "s to align uwb_epoch start\n",
             diff);
    ztimer_sleep(ZTIMER_EPOCH, diff);
    event_periodic_start(&uwb_epoch_end, CONFIG_EBID_ROTATION_T_S);
    /* bootstrap new uwb_epoch */
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
        event_timeout_clear(&_silent_timeout);
        event_periodic_stop(&uwb_epoch_end);
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

static int _cmd_id(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("dwm1001 id: %s\n", state_manager_get_id());
    return 0;
}

static const shell_command_t _commands[] = {
    { "id", "Print device id", _cmd_id },
    { NULL, NULL, NULL }
};

int main(void)
{
#if defined(MODULE_PERIPH_GPIO_IRQ) && defined(BTN0_PIN)
    /* initialize a button to manually trigger an update */
    gpio_init_int(BTN0_PIN, BTN0_MODE, GPIO_FALLING, _declare_positive, NULL);
#endif
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
    /* initialize remote endpoint endpoint */
    coap_init_remote(&remote, CONFIG_PEPPER_SERVER_ADDR,
                     CONFIG_PEPPER_SERVER_PORT);
#endif
    event_timeout_ztimer_init(&_silent_timeout, ZTIMER_EPOCH,
                              EVENT_PRIO_HIGHEST,
                              (event_t *)&_scan_adv_start_ev);
    /* init global state manager */
    state_manager_init();
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
    state_manager_set_remote(&remote);
#endif
#if IS_USED(MODULE_STATE_MANAGER_SECURITY)
    state_manager_security_init(EVENT_PRIO_MEDIUM);
#endif

    /* init encounters manager */
    uwb_ed_memory_manager_init(&manager);
    uwb_ed_list_init(&uwb_ed_list, &manager, &ebid);
#if IS_USED(MODULE_UWB_ED_BPF)
    uwb_ed_bpf_init();
#endif
    /* init twr */
    twr_event_mem_manager_init(&twr_manager);
    twr_managed_set_manager(&twr_manager);
    twr_init(EVENT_PRIO_HIGHEST);
    twr_register_rng_cb(_twr_complete_cb);
    /* init ble advertiser */
    desire_ble_adv_init(EVENT_PRIO_HIGHEST);
    desire_ble_adv_set_cb(_adv_cb);
    /* init ble scanner and current_time */
    desire_ble_scan_init(&desire_ble_scanner_params, _detection_cb);
    current_time_init();
    current_time_hook_init(&_pre_hook, _pre_adjust_time, &_epoch_reset);
    current_time_hook_init(&_post_hook, _post_adjust_time, &_epoch_reset);
    current_time_add_pre_cb(&_pre_hook);
    current_time_add_post_cb(&_post_hook);
    /* begin and align epoch */
    _aligned_epoch_start();
    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
