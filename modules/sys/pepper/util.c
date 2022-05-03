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

#include "pepper.h"
#include "fmt.h"
#include "luid.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

#define PEPPER_UID_STRLEN   (2 + PEPPER_UID_LEN * 2)

/* 0xXXXX */
static uint8_t _uid[PEPPER_UID_LEN];
/* DWXXXX + '\0' */
static char _uid_str[PEPPER_UID_STRLEN + 1];
/* DWXXXX + ':' + buffer + '\0' */
static char _base_name[PEPPER_UID_STRLEN + 1 + CONFIG_PEPPER_BASE_NAME_BUFFER + 1];

void pepper_uid_init(void)
{
#ifndef BOARD_NATIVE
    luid_get(_uid, PEPPER_UID_LEN);
    _uid_str[0] = 'D';
    _uid_str[1] = 'W';
    fmt_bytes_hex(&_uid_str[2], _uid, PEPPER_UID_LEN);
    _uid_str[PEPPER_UID_STRLEN] = '\0';
#else
    memcpy(_uid_str, "DW1234", sizeof("DW1234"));
#endif
    /* set initial base name to node uid */
    memcpy(_base_name, _uid_str, sizeof(_uid_str));
}

uint8_t* pepper_get_uid(void)
{
    return _uid;
}

char *pepper_get_uid_str(void)
{
    return _uid_str;
}

int pepper_set_serializer_bn(char *base_name)
{
    int len =  strlen(base_name);

    if (len > CONFIG_PEPPER_BASE_NAME_BUFFER || !len) {
        return -1;
    }
    _base_name[strlen(_uid_str)] = ':';
    memcpy(_base_name + strlen(_uid_str) + 1, base_name, strlen(base_name));
    _base_name[strlen(base_name) + 1 + strlen(_uid_str)] = '\0';
    return 0;
}

const char *pepper_get_serializer_bn(void)
{
    return _base_name;
}
