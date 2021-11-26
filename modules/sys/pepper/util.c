/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_pepper
 * @{
 *
 * @file
 * @brief       PrEcise Privacy-PresERving Proximity Tracing (PEPPER) implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */
#include "fmt.h"
#include "luid.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

/**
 * @brief   Token ID length in bytes
 */
#define PEPPER_UID_LEN          (2)

static char _uid[sizeof("DW") + PEPPER_UID_LEN * 2 + 1];

void pepper_uid_init(void)
{
#ifndef BOARD_NATIVE
    uint8_t addr[PEPPER_UID_LEN];
    luid_get(addr, PEPPER_UID_LEN);
    fmt_bytes_hex(&_uid[(sizeof("DW") - 1)], addr, PEPPER_UID_LEN);
    _uid[0] = 'D';
    _uid[1] = 'W';
    _uid[sizeof("DW") + 2 * PEPPER_UID_LEN] = '\0';
#else
    memcpy(_uid, "DW1234", sizeof("DW1234"));
#endif
}

char *pepper_get_uid(void)
{
    return _uid;
}


