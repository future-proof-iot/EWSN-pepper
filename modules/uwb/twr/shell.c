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
 * @brief       TWR shell commands
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

#include "twr.h"

static void _print_usage(void)
{
    puts("Usage:");
    puts("\ttwr get win: returns the listen window in us");
    puts("\ttwr set <win_us>: sets listen window in us, win_us < UINT16_MAX");
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
        }
        _print_usage();
        return -1;
    }

    if (!strcmp(argv[1], "status")) {
        puts("  twr:");
        printf("    mem: %d/%d (free/total)\n",
            memarray_available(twr_managed_get_manager()), CONFIG_TWR_EVENT_BUF_SIZE);
    }
    _print_usage();
    return -1;
}

SHELL_COMMAND(twr, "proximity tracing twr", _twr_handler);
