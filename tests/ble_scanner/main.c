/*
 * Copyright (C) 2022 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       BLE Scanner Test Application
 *
 * @}
 */

#include <stdio.h>
#include <stdlib.h>

#include "shell.h"
#include "shell_commands.h"

#include "ble_scanner.h"
#include "ble_scanner_params.h"
#include "ble_scanner_netif_params.h"
#include "current_time.h"

int main(void)
{
    puts("Generic BLE Scanner Test Application");

    /* initialize the ble scanner */
    ble_scanner_init(&ble_scan_params[1]);
    /* initializer current_time */
    current_time_init();
    ble_scanner_netif_init();

    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
