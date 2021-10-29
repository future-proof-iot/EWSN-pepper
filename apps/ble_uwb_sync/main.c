#include <stdio.h>
#include <stdlib.h>

#include "timex.h"
#include "shell.h"
#include "shell_commands.h"
#include "event/thread.h"

#include "ztimer.h"
#include "twr.h"
#include "ebid.h"
#include "desire_ble_adv.h"
#include "desire_ble_scan.h"
#include "desire_ble_scan_params.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

/* default scan duration (20s) */
#define DEFAULT_OFFSET_MS         (1000U)
#define DEFAULT_TOF_COMPENSATION  (10U)
#define DEFAULT_DURATION_MS       BLE_HS_FOREVER

static twr_event_mem_manager_t twr_manager;
static ebid_t ebid;
static uint16_t tof_offset = DEFAULT_OFFSET_MS;

void _scan_cb(uint32_t ticks,
              const ble_addr_t *addr, int8_t rssi,
              const desire_ble_adv_payload_t *adv_payload)
{
    (void)addr;
    (void)rssi;
    (void)ticks;
    uint32_t cid;
    uint8_t sid;
    uint32_t now_ticks = ztimer_now(ZTIMER_USEC);
    decode_sid_cid(adv_payload->data.sid_cid, &sid, &cid);
    twr_schedule_listen_managed(tof_offset);
    LOG_INFO("%" PRIu32 " [scan_cb] cid=%" PRIx16 "\n", ztimer_now(ZTIMER_MSEC), (uint16_t)cid);
    printf("scan delay: %"PRIu32"\n", now_ticks - ticks);
}

static void _adv_cb(uint32_t ticks, void *arg)
{
    (void)arg;
    (void)ticks;
    uint32_t now_ticks = ztimer_now(ZTIMER_USEC);
    twr_schedule_request_managed(0xCAFE, tof_offset);
    LOG_INFO("%" PRIu32 " [ble/uwb]: adv_cb\n", ztimer_now(ZTIMER_MSEC));
    printf("adv delay: %"PRIu32"\n", now_ticks - ticks);
}

static void _twr_cb(twr_event_data_t *data)
{
    (void)data;
    LOG_INFO("%" PRIu32 " [ble/uwb]: addr=(0x%" PRIx16 "), d=(%" PRIu16 "cm)\n",
             ztimer_now(ZTIMER_MSEC), data->addr, data->range);
}


int _cmd_scan_start(int argc, char **argv)
{
    uint32_t duration = DEFAULT_DURATION_MS;

    if ((argc == 2) && (memcmp(argv[1], "help", 4) == 0)) {
        printf("usage: %s [duration in ms]\n", argv[0]);
        return 0;
    }
    if (argc >= 2) {
        duration = (uint32_t)(atoi(argv[1]));
    }
    printf("Scanning for %" PRIu32 "\n", duration);
    desire_ble_scan_start(duration);

    return 0;
}

int _cmd_scan_stop(int argc, char **argv)
{
    desire_ble_scan_stop();

    (void)argc;
    (void)argv;
    return 0;
}

int _cmd_adv_start(int argc, char **argv)
{
    uint32_t rotation = CONFIG_SLICE_ROTATION_T_S;
    uint32_t duration = CONFIG_EBID_ROTATION_T_S;

    if ((argc == 2) && (memcmp(argv[1], "help", 4) == 0)) {
        printf("usage: %s <slice advertisment duration in ms> <ebid advertisment duration in ms>\n", argv[0]);
        printf("default: %s %d %ld", argv[0], CONFIG_SLICE_ROTATION_T_S, CONFIG_EBID_ROTATION_T_S / 60);
        return 0;
    }
    if (argc >= 3) {
        rotation = (uint32_t)(atoi(argv[1]));
        duration = (uint32_t)(atoi(argv[2]));
    }
    desire_ble_adv_start(&ebid, 1000, duration * 1000, rotation);

    /* set new short addr */
    twr_set_short_addr(0xCAFE);

    return 0;
}

int _cmd_adv_stop(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    desire_ble_adv_stop();
    printf("Stopped ongoing advertisements (if any)\n");

    return 0;
}


static void _print_usage(void)
{
    puts("Usage:");
    puts("\ttwr set: %");
}


static int _twr_handler(int argc, char **argv)
{
    if (argc < 2) {
        _print_usage();
        return -1;
    }

    if (!strcmp(argv[1], "set")) {
        if (argc >= 3) {
            if (!strcmp(argv[2], "win")) {
                twr_set_listen_window(atoi(argv[3]));
                return 0;
            }
            else if (!strcmp(argv[2], "offset")) {
                tof_offset = atoi(argv[3]);
                return 0;
            }
        }
        _print_usage();
        return -1;
    }

    if (!strcmp(argv[1], "get")) {
        if (argc == 3) {
            if (!strcmp(argv[2], "win")) {
                printf("twr listen win: %d\n", twr_get_listen_window());
                return 0;
            }
            else if (!strcmp(argv[2], "offset")) {
                printf("tof offset: %d\n", tof_offset);
                return 0;
            }
        }
        _print_usage();
        return -1;
    }


    _print_usage();
    return -1;
}


static const shell_command_t _commands[] = {
    { "twr", "twr shell handler", _twr_handler},
    { "scan_start", "trigger a BLE scan of Desire packets", _cmd_scan_start },
    { "scan_stop", "stops scanning of Desire packets", _cmd_scan_stop },
    { "adv_start", "trigger a BLE advertisment of Desire packets", _cmd_adv_start },
    { "adv_stop", "trigger a BLE advertisment of Desire packets", _cmd_adv_stop },
    { NULL, NULL, NULL }
};

int main(void)
{
    puts("Desire BLE Scanner Test Application");

    /* init ble advertiser */
    desire_ble_adv_init(EVENT_PRIO_MEDIUM);
    desire_ble_adv_set_cb(_adv_cb);
    /* init ble scanner and current_time */
    desire_ble_scan_init(&desire_ble_scanner_params, _scan_cb);
    /* init twr */
    twr_event_mem_manager_init(&twr_manager);
    twr_managed_set_manager(&twr_manager);
    twr_init(EVENT_PRIO_HIGHEST);
    twr_register_rng_cb(_twr_cb);
    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
