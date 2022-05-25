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
 * @brief       Current Time shell commands
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

#include "current_time.h"
#include "periph/rtc.h"
#include "ztimer.h"

static void _print_usage(void)
{
    puts("Usage:");
    puts("\ttime <year> <month=[1 = January, 12 = December]> <day> <hour> "
         "<min> <sec>");
}
static int _cmd_time(int argc, char **argv)
{
    struct tm time;
    uint32_t epoch;

    if (argc == 1) {
        epoch = current_time_get();
        rtc_localtime(epoch, &time);
    }
    else if (argc == 2) {
        epoch = (uint32_t)atoi(argv[1]);
        printf("[current_time] shell: new Epoch: %" PRIu32 "\n", epoch);
        current_time_update(epoch);
    }
    else if (argc == 7) {
        time.tm_year = (uint16_t)(atoi(argv[1])) - 1900;
        time.tm_mon = (uint8_t)(atoi(argv[2])) - 1;
        time.tm_mday = (uint8_t)(atoi(argv[3]));
        time.tm_hour = (uint8_t)(atoi(argv[4]));
        time.tm_min = (uint8_t)(atoi(argv[5]));
        time.tm_sec = (uint8_t)(atoi(argv[6]));
        epoch = rtc_mktime(&time);
        current_time_update(rtc_mktime(&time));
    }
    else {
        _print_usage();
        return -1;
    }
    printf("[current_time] shell:\n");
    printf("\tDate: %04d-%02d-%02d %02d:%02d:%02d\n",
           time.tm_year + 1900,
           time.tm_mon + 1,
           time.tm_mday,
           time.tm_hour,
           time.tm_min,
           time.tm_sec);
    printf("\tEpoch: %" PRIu32 "\n", epoch);
    return 0;
}

SHELL_COMMAND(time, "time setting utility", _cmd_time);
