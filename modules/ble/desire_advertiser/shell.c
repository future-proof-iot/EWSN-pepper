/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_desire_ble_adv
 * @{
 *
 * @file
 * @brief       DESIRE based ble advertisement module shell commands
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "kernel_defines.h"
#include "ztimer.h"
#include "shell.h"
#include "shell_commands.h"
#include "xfa.h"

#include "random.h"
#include "desire_ble_adv.h"
#include "pepper.h"

#define BLE_ADV_MAX_EVENTS_DEFAULT  ((CONFIG_EBID_ROTATION_T_S * MS_PER_SEC) / \
                                     CONFIG_BLE_ADV_INTERVAL_MS)

/* dummy ebid */
static ebid_t _ebid;

static void _print_usage(void)
{
    puts("Usage:");
    puts("\tadv start [-r <advs per slice>] [-c <advs count>] [-i <ms interval>] [-h] ");
    printf("\t - advs per slice: x advertisements per EBID slice"
           " (default: %" PRIu32 ")\n", CONFIG_ADV_PER_SLICE);
    printf("\t - advs count: number of advertisements to send"
           " (default: %" PRIu32 ")\n", BLE_ADV_MAX_EVENTS_DEFAULT);
    printf("\t - ms interval: the delay between advertisements"
           " (default: %" PRIu32 ")\n", CONFIG_BLE_ADV_INTERVAL_MS);
    puts("\tadv stop: stop ongoing advertisements if any");
    puts("\tadv get cid: returns current cid");
    puts("\tadv set <cid>: sets new cid");
}

static int _adv_handler(int argc, char **argv)
{
    if (argc < 2) {
        _print_usage();
        return -1;
    }

    if (!strcmp(argv[1], "start")) {
        uint32_t itvl_ms = CONFIG_BLE_ADV_INTERVAL_MS;
        uint32_t advs_max = BLE_ADV_MAX_EVENTS_DEFAULT;
        uint32_t advs_slice = CONFIG_ADV_PER_SLICE;
        int res = 0;

        /* parse command line arguments */
        for (int i = 2; i < argc; i++) {
            char *arg = argv[i];
            switch (arg[1]) {
            case 'c':
                if ((i++) < argc) {
                    advs_max = (uint32_t)atoi(argv[i]);
                    continue;
                }
            /* intentionally falls through */
            case 'h':
                res = 1;
                continue;
            /* intentionally falls through */
            case 'i':
                if ((++i) < argc) {
                    itvl_ms = (uint32_t)atoi(argv[i]);
                    continue;
                }
            /* intentionally falls through */
            case 'r':
                if ((++i) < argc) {
                    advs_slice = (uint32_t)atoi(argv[i]);
                    continue;
                }
            /* intentionally falls through */
            default:
                res = 1;
                break;
            }
        }
        if (res != 0) {
            _print_usage();
            return 1;
        }
        if (_ebid.status.status != EBID_HAS_ALL) {
            uint8_t pk[C25519_KEY_SIZE];
            random_bytes(pk, C25519_KEY_SIZE);
            ebid_generate_from_pk(&_ebid, pk);

        }
        printf("[adv] shell: start adertising\n");
        desire_ble_adv_start(&_ebid, itvl_ms, advs_max, advs_slice);
        if (IS_ACTIVE(CONFIG_BLE_ADV_SHELL_BLOCKING)) {
            ztimer_sleep(ZTIMER_MSEC, itvl_ms * advs_max);
        }
        return 0;
    }

    if (!strcmp(argv[1], "stop")) {
        desire_ble_adv_stop();
        printf("[adv] shell: advertisement stopped");
        return -1;
    }

    if (!strcmp(argv[1], "set")) {
        if (argc == 4) {
            if (!strcmp(argv[2], "cid")) {
                desire_ble_adv_set_cid(atoi(argv[3]));
                printf("[adv] shell: set cid to 0x%08"PRIx32"\n", desire_ble_adv_get_cid());
                return 0;
            }
        }
        _print_usage();
        return -1;
    }

    if (!strcmp(argv[1], "get")) {
        if (argc == 3) {
            if (!strcmp(argv[2], "cid")) {
                printf("[adv] shell: cid 0x%08"PRIx32"\n", desire_ble_adv_get_cid());
                return 0;
            }
        }
        _print_usage();
        return -1;
    }

    _print_usage();
    return -1;
}

SHELL_COMMAND(adv, "proximity tracing ble advertiser", _adv_handler);
