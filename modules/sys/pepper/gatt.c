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

#include "memarray.h"

#include "ble_rng_gatt.h"
#include "ble_rss_scan.h"

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/ble_gatt.h"
#include "net/bluetil/ad.h"

#include "pepper.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

/* UUID = 3878e9c0-0000-1000-8000-00805f9b34fb , UWB ranging service */
static const ble_uuid_t *gatt_svr_svc_pepper_uuid
    = BLE_UUID128_DECLARE(0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10,
                          0x00, 0x00, 0xc0, 0xe9, 0x78, 0x38);

/* UUID = 4dba712c-2f2f-11eb-adc1-0242ac120001 */
static const ble_uuid_t *gatt_svr_chr_pepper_notif_uuid
    = BLE_UUID128_DECLARE(0x01, 0x00, 0x12, 0xac, 0x42, 0x02, 0xc1, 0xad, 0xeb,
                          0x11, 0x2f, 0x2f, 0x2c, 0x71, 0xba, 0x4d);


/* UUID = 4dba712c-2f2f-11eb-adc1-0242ac120002 */
static const ble_uuid_t *gatt_svr_chr_pepper_config_uuid
    = BLE_UUID128_DECLARE(0x02, 0x00, 0x12, 0xac, 0x42, 0x02, 0xc1, 0xad, 0xeb,
                          0x11, 0x2f, 0x2f, 0x2c, 0x71, 0xba, 0x4d);

/* UUID = 4dba712c-2f2f-11eb-adc1-0242ac120003 */
static const ble_uuid_t *gatt_svr_chr_pepper_start_uuid
    = BLE_UUID128_DECLARE(0x03, 0x00, 0x12, 0xac, 0x42, 0x02, 0xc1, 0xad, 0xeb,
                          0x11, 0x2f, 0x2f, 0x2c, 0x71, 0xba, 0x4d);

/* define the bluetooth services for our device */
/* GATT service definitions */
static const struct ble_gatt_svc_def gatt_svr_svcs[] =
{
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = gatt_svr_svc_pepper_uuid,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = gatt_svr_chr_pepper_start_uuid,
                .access_cb = _pepper_start_handler,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ,
            },
            {
                /* Characteristic: Read/Write UWB config */
                .uuid = gatt_svr_chr_pepper_config_uuid,
                .access_cb = _pepper_cfg_handler,
                .flags =  BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },

            {
                .uuid = gatt_svr_chr_pepper_notif_uuid,
                .val_handle = &_pepper_notif_handle,
                .access_cb = _dummy_handler,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            },
            {
                0,     /* No more characteristics in this service */
            },
        }
    },
    {
        0,     /* No more services */
    },
}

static int _pepper_cfg_handler(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    int rc = 0;

    switch (ctxt->op) {

    case BLE_GATT_ACCESS_OP_READ_CHR:
        LOG_INFO("read char\n");
        /* send given data to the client */
        rc = os_mbuf_append(ctxt->om, (uint8_t *)pepper_get_base_name(),
                            strlen(pepper_get_base_name()) + 1);
        break;
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        LOG_INFO("write char\n");
        uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
        if (om_len > CONFIG_PEPPER_BASE_NAME_BUFFER) {
            rc = 1;
        }
        else {
            rc = ble_hs_mbuf_to_flat(ctxt->om, pepper_get_base_name(),
                                     CONFIG_PEPPER_BASE_NAME_BUFFER,
                                     &om_len);
        }
        break;
    case BLE_GATT_ACCESS_OP_READ_DSC:
        LOG_INFO("read from descriptor\n");
        break;
    case BLE_GATT_ACCESS_OP_WRITE_DSC:
        LOG_INFO("write to descriptor\n");
        break;
    default:
        LOG_INFO("unhandled operation!\n");
        rc = 1;
        break;
    }

    return rc;
}

void peppet_gat_init(void)
{
    int rc = 0;

    /* verify and add our custom services */
    rc = ble_gatts_count_cfg(_gatt_svr_svcs);
    assert(rc == 0);
    rc = ble_gatts_add_svcs(_gatt_svr_svcs);
    assert(rc == 0);

    /* reload the GATT server to link our added services */
    ble_gatts_start();

    /* register gap event listener */
    rc = ble_gap_event_listener_register(&_gap_event_listener, _gap_event_cb, NULL);
    assert(rc == 0);

    /* fix compilation error when using DEVELHELP=0 */
    (void)rc;
}
