/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @brief       PEPPER shell commands
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "shell.h"
#include "shell_commands.h"
#include "xfa.h"

#include "desire_ble_adv.h"
#include "pepper.h"
#if IS_USED(MODULE_TWR)
#include "twr.h"
#endif
#include "ed.h"

static void _print_usage(void)
{
    puts("Usage:");
    puts("\tpepper status: controller status information");
    puts("\tpepper start [-d <epoch duration s>] [-i <ms interval>] [-r <advs per slice>]"
         " [-a (align epoch) [-c <iterations> ] [-s <win_ms,itvl_ms>]");
    puts("\tpepper stop: stop proximity tracing");
    puts("\tpepper set bn <base name>: sets base name for logging");
    puts("\tpepper get bn: returns base name for logging");
    puts("\tpepper get uid: returns unique identifier");
#if IS_USED(MODULE_TWR)
    puts("\tpepper twr get win: returns the listen window in us");
    puts("\tpepper twr set <win_us>: sets listen window in us, win_us < UINT16_MAX");
    printf("\tpepper twr set rx_off <offset ticks>: offset > -%" PRIu32 "\n",
           CONFIG_TWR_MIN_OFFSET_TICKS);
    printf("\tpepper twr set tx_off <offset ticks>: offset > -%" PRIu32 "\n",
           CONFIG_TWR_MIN_OFFSET_TICKS);
#endif
}

#if IS_USED(MODULE_TWR)
static int _twr_handler(int argc, char **argv)
{
    if (argc < 2) {
        _print_usage();
        return -1;
    }

    if (!strcmp(argv[1], "reset")) {
        twr_reset();
        return -1;
    }

    if (!strcmp(argv[1], "set")) {
        if (argc >= 3) {
            if (!strcmp(argv[2], "win")) {
                twr_set_listen_window(atoi(argv[3]));
                return 0;
            }
            if (!strcmp(argv[2], "rx_off")) {
                pepper_twr_set_rx_offset(atoi(argv[3]));
                return 0;
            }
            if (!strcmp(argv[2], "tx_off")) {
                pepper_twr_set_tx_offset(atoi(argv[3]));
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
            if (!strcmp(argv[2], "rx_off")) {
                printf("twr rx offset: %d\n", pepper_twr_get_rx_offset());
                return 0;
            }
            if (!strcmp(argv[2], "tx_off")) {
                printf("twr tx offset: %d\n", pepper_twr_get_tx_offset());
                return 0;
            }
        }
        _print_usage();
        return -1;
    }
    return 0;
}
#endif

static int _parse_scan_params(char *arg, uint32_t *win_ms, uint32_t *itvl_ms)
{
    char *value = strtok(arg, ",");

    if (value) {
        *win_ms = atoi(value);
        value = strtok(NULL, ",");
        if (value) {
            *itvl_ms = atoi(value);
            return 0;
        }
    }
    return -1;
}

static int _pepper_handler(int argc, char **argv)
{
    if (argc < 2) {
        _print_usage();
        return -1;
    }

    if (!strcmp(argv[1], "start")) {
        pepper_start_params_t params = {
            .epoch_duration_s = CONFIG_EPOCH_DURATION_SEC,
            .epoch_iterations = 0,
            .adv_itvl_ms = CONFIG_BLE_ADV_ITVL_MS,
            .advs_per_slice = CONFIG_ADV_PER_SLICE,
            .scan_itvl_ms = CONFIG_BLE_SCAN_ITVL_MS,
            .scan_win_ms = CONFIG_BLE_SCAN_WIN_MS,
            .align = false,
        };
        int res = 0;

        /* parse command line arguments */
        for (int i = 2; i < argc; i++) {
            char *arg = argv[i];
            switch (arg[1]) {
            case 'd':
                if ((i++) < argc) {
                    params.epoch_duration_s = (uint32_t)atoi(argv[i]);
                    continue;
                }
            /* intentionally falls through */
            case 'h':
                res = 1;
                continue;
            /* intentionally falls through */
            case 'i':
                if ((++i) < argc) {
                    params.adv_itvl_ms = (uint32_t)atoi(argv[i]);
                    continue;
                }
            /* intentionally falls through */
            case 'c':
                if ((++i) < argc) {
                    params.epoch_iterations = (uint32_t)atoi(argv[i]);
                    continue;
                }
            /* intentionally falls through */
            case 'r':
                if ((++i) < argc) {
                    params.advs_per_slice = (uint32_t)atoi(argv[i]);
                    continue;
                }
            /* intentionally falls through */
            case 'a':
                params.align = true;
                continue;
            /* intentionally falls through */
            case 's':
                if ((++i) < argc) {
                    uint32_t scan_itvl_ms = 0;
                    uint32_t scan_win_ms = 0;
                    _parse_scan_params(argv[i], &scan_win_ms, &scan_itvl_ms);
                    params.scan_itvl_ms = scan_itvl_ms;
                    params.scan_win_ms = scan_win_ms;
                    continue;
                }
            /* intentionally falls through */
            default:
                res = 1;
                break;
            }
        }

        if (params.scan_itvl_ms < params.scan_win_ms) {
            res = 1;
        }

        if (res != 0) {
            _print_usage();
            return 1;
        }
        puts("[pepper] shell: start proximity tracing");
        printf("\tepoch_duration: %" PRIu32 "[s]\n", params.epoch_duration_s);
        if (params.epoch_iterations != 0 && params.epoch_iterations != UINT32_MAX) {
            printf("\tepoch_iterations: %" PRIu32 "\n", params.epoch_iterations);
        }
        else {
            printf("\tepoch_iterations: until stopped\n");
        }
        printf("\tadv_per_slice: %" PRIu32 "\n", params.advs_per_slice);
        printf("\tadv_itvl: %" PRIu32 "[ms]\n", params.adv_itvl_ms);
        printf("\tscan_itvl: %" PRIu32 "[ms]\n", params.scan_itvl_ms);
        printf("\tscan_win: %" PRIu32 "[ms]\n", params.scan_win_ms);
        pepper_start(&params);
        if (IS_ACTIVE(CONFIG_PEPPER_SHELL_BLOCKING)) {
            /* if iterations == 0 the loop will not run and this wont't block
               which is wanted */
            for (uint32_t i = 0; i < params.epoch_iterations; i++) {
                ztimer_sleep(ZTIMER_MSEC, params.epoch_duration_s * MS_PER_SEC);
            }
        }
        return 0;
    }

    if (!strcmp(argv[1], "get")) {
        if (argc >= 2) {
            if (!strcmp(argv[2], "time")) {
                printf("[current_time]: epoch\n");
                printf("\tcurrent:     %" PRIu32 "\n", ztimer_now(ZTIMER_EPOCH));
                return 0;
            }
            if (IS_USED(MODULE_PEPPER_UTIL)) {
                if (!strcmp(argv[2], "uid")) {
                    printf("[pepper]: uid %s\n", pepper_get_uid());
                    return 0;
                }
                if (!strcmp(argv[2], "bn")) {
                    printf("[pepper]: base name %s\n", pepper_get_serializer_bn());
                    return 0;
                }
            }
        }
        _print_usage();
        return -1;
    }

    if (!strcmp(argv[1], "stop")) {
        pepper_stop();
        puts("[pepper] shell: stop proximity tracing");
        return 0;
    }

    if (!strcmp(argv[1], "status")) {
        printf("status: ");
        if (pepper_is_active()) {
            printf("active\n");
        }
        else {
            printf("idle\n");
        }
#if IS_USED(MODULE_TWR)
        puts("  twr:");
        printf("    mem: %d/%d (free/total)\n",
               memarray_available(&pepper_get_controller()->twr_mem.mem),
               CONFIG_TWR_EVENT_BUF_SIZE);
#endif
        puts("  ed:");
        printf("    mem: %d/%d (free/total)\n",
               memarray_available(&pepper_get_controller()->ed_mem.mem),
               CONFIG_ED_BUF_SIZE);

        return 0;
    }

#if IS_USED(MODULE_TWR)
    if (!strcmp(argv[1], "twr")) {
        return _twr_handler(argc - 1, &argv[1]);
    }
#endif

    if (!strcmp(argv[1], "set")) {
        if (argc >= 3) {
            if (!strcmp(argv[2], "bn")) {
                if (pepper_set_serializer_bn(argv[3]) == 0) {
                    puts("[pepper] shell: set basename");
                    return 0;
                }
                puts("[pepper] shell: error, basename is too long");
                return -1;
            }
        }
        _print_usage();
        return -1;
    }


    _print_usage();
    return -1;
}

SHELL_COMMAND(pepper, "proximity tracing configuration", _pepper_handler);
