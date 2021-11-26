/*
 * Copyright (C) 2020 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     pepper
 * @{
 *
 * @file
 * @brief       Example of twr ranging exposed over BLE service.
 *
 * @author      Roudy DAGHER <roudy.dagher@inria.fr>
 *
 * @}
 */

#include <stddef.h>

#include "nimble_autoadv.h"

#include "host/ble_hs.h"
#include "host/ble_gatt.h"
#include "host/util/util.h"
#include "net/bluetil/ad.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "pepper.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

extern void stdio_nimble_init(void);

void pepper_stdio_nimble_init(void)
{
    int rc = 0;

    pepper_uid_init();
    char *uid = pepper_get_uid();
    /* set the device name */
    ble_svc_gap_device_name_set(uid);

    /* init stdio_nimble */
    stdio_nimble_init();

    /* start to advertise this node with its name */
    nimble_autoadv_add_field(BLE_GAP_AD_NAME_SHORT, uid, strlen(uid));
    nimble_autoadv_start(NULL);

    assert(rc == 0);

    /* fix compilation error when using DEVELHELP=0 */
    (void)rc;
}
