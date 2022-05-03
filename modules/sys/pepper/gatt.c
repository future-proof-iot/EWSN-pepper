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

#include "desire_ble_adv.h"
#include "pepper.h"
#include "current_time.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

/* UUID = 5ea4f59a-78da-499d-931d-cfbd25ed365b, UWB ranging service */
static const ble_uuid128_t gatt_svr_svc_pepper_uuid
    = BLE_UUID128_INIT(0x5b, 0x36, 0xed, 0x25, 0xbd, 0xcf, 0x1d,
                       0x93, 0x49, 0x9d, 0x78, 0xda, 0x5e, 0xa4,
                       0xf5, 0x9);

/* UUID = ce2ae1d5-b067-c348-985a-ba2f2b0ac8d0 */
static const ble_uuid128_t gatt_svr_chr_pepper_config_uuid
    = BLE_UUID128_INIT(0xd0, 0xc8, 0x0a, 0x2b, 0x2f, 0xba, 0x5a, 0x98,
                       0x48, 0xc3, 0x67, 0xb0, 0xd5, 0xe1, 0x2a, 0xce);

/* UUID = ce2ae1d5-b067-c348-985a-ba2f2b0ac8d1 */
static const ble_uuid128_t gatt_svr_chr_pepper_start_uuid
    = BLE_UUID128_INIT(0xd1, 0xc8, 0x0a, 0x2b, 0x2f, 0xba, 0x5a, 0x98,
                       0x48, 0xc3, 0x67, 0xb0, 0xd5, 0xe1, 0x2a, 0xce);

/* UUID = ce2ae1d5-b067-c348-985a-ba2f2b0ac8d2 */
static const ble_uuid128_t gatt_svr_chr_pepper_stop_uuid
    = BLE_UUID128_INIT(0xd2, 0xc8, 0x0a, 0x2b, 0x2f, 0xba, 0x5a, 0x98,
                       0x48, 0xc3, 0x67, 0xb0, 0xd5, 0xe1, 0x2a, 0xce);

/* UUID = ce2ae1d5-b067-c348-985a-ba2f2b0ac8d3 */
static const ble_uuid128_t gatt_svr_chr_pepper_restart_uuid
    = BLE_UUID128_INIT(0xd3, 0xc8, 0x0a, 0x2b, 0x2f, 0xba, 0x5a, 0x98,
                       0x48, 0xc3, 0x67, 0xb0, 0xd5, 0xe1, 0x2a, 0xce);

/* service handlers */
static int _pepper_cfg_handler(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);
static int _devinfo_handler(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg);
static int _pepper_start_handler(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg);
static int _pepper_stop_handler(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);
static int _pepper_restart_handler(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt *ctxt, void *arg);
#if IS_USED(MODULE_PEPPER_CURRENT_TIME)
static int _pepper_current_time_handler(uint16_t conn_handle, uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt *ctxt, void *arg);
#endif

/* define the bluetooth services for our device */
/* GATT service definitions */
static const struct ble_gatt_svc_def gatt_svr_svcs[] =
{
    {
        /* Device Information Service */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_GATT_SVC_DEVINFO),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(BLE_GATT_CHAR_MANUFACTURER_NAME),
                .access_cb = _devinfo_handler,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = BLE_UUID16_DECLARE(BLE_GATT_CHAR_MODEL_NUMBER_STR),
                .access_cb = _devinfo_handler,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                0, /* no more characteristics in this service */
            },
        }
    },
    {
        /* PEPPER Information Service */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = (ble_uuid_t *)&gatt_svr_svc_pepper_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                /* Characteristic: Read/Write PEPPER config */
                .uuid = (ble_uuid_t *)&gatt_svr_chr_pepper_config_uuid.u,
                .access_cb = _pepper_cfg_handler,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            {
                /* Characteristic: Start PEPPER */
                .uuid = (ble_uuid_t *)&gatt_svr_chr_pepper_start_uuid.u,
                .access_cb = _pepper_start_handler,
                .flags =  BLE_GATT_CHR_F_WRITE,
            },
            {
                /* Characteristic: Stop PEPPER */
                .uuid = (ble_uuid_t *)&gatt_svr_chr_pepper_stop_uuid.u,
                .access_cb = _pepper_stop_handler,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                /* Characteristic: retart PEPPER */
                .uuid = (ble_uuid_t *)&gatt_svr_chr_pepper_restart_uuid.u,
                .access_cb = _pepper_restart_handler,
                .flags =  BLE_GATT_CHR_F_WRITE,
            },
            {
                0,     /* No more characteristics in this service */
            },
        }
    },
#if IS_USED(MODULE_PEPPER_CURRENT_TIME)
    {
        /* Service: Device Information */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(CURRENT_TIME_SERVICE_UUID16),
        .characteristics = (struct ble_gatt_chr_def[]) { {
                                                             /* Characteristic: * Manufacturer name */
                                                             .uuid = BLE_UUID16_DECLARE(
                                                                 CURRENT_TIME_UUID16),
                                                             .access_cb =
                                                                 _pepper_current_time_handler,
                                                             .flags = BLE_GATT_CHR_F_READ |
                                                                      BLE_GATT_CHR_F_WRITE,
                                                         },
                                                         {
                                                             0, /* No more characteristics in this service */
                                                         }, }
    },
#endif
    {
        0,     /* No more services */
    },
};

static const char *_manufacturer = "RIOT";
static const char *_model_number = RIOT_BOARD ":" RIOT_MCU;
static int _devinfo_handler(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;

    const char *str;

    switch (ble_uuid_u16(ctxt->chr->uuid)) {
    case BLE_GATT_CHAR_MANUFACTURER_NAME:
        str = _manufacturer;
        break;
    case BLE_GATT_CHAR_MODEL_NUMBER_STR:
        str = _model_number;
        break;
    default:
        return BLE_ATT_ERR_UNLIKELY;
    }

    int res = os_mbuf_append(ctxt->om, str, strlen(str));

    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
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
        LOG_INFO("[pepper] gatt: cfg read bn=(\"%s\")\n", pepper_get_serializer_bn());
        rc = os_mbuf_append(ctxt->om, (uint8_t *)pepper_get_serializer_bn(),
                            strlen(pepper_get_serializer_bn()));
        break;
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        LOG_INFO("[pepper] gatt: cfg write...\n");
        uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
        if (om_len > CONFIG_PEPPER_BASE_NAME_BUFFER) {
            LOG_INFO("error, exceeded buffer size\n");
            rc = 1;
        }
        else {
            char new_name[CONFIG_PEPPER_BASE_NAME_BUFFER + 1];
            rc = ble_hs_mbuf_to_flat(ctxt->om, new_name,
                                     CONFIG_PEPPER_BASE_NAME_BUFFER,
                                     &om_len);
            /* we need to null-terminate the received string */
            new_name[om_len] = '\0';
            pepper_set_serializer_bn(new_name);
            LOG_INFO("bn=(\"%s\")\n", pepper_get_serializer_bn());
        }
        break;
    case BLE_GATT_ACCESS_OP_READ_DSC:
        LOG_DEBUG("[pepper] gatt: cfg read from descriptor\n");
        break;
    case BLE_GATT_ACCESS_OP_WRITE_DSC:
        LOG_DEBUG("[pepper] gatt: cfg write to descriptor\n");
        break;
    default:
        LOG_WARNING("[pepper] gatt: cfg unhandled operation!\n");
        rc = 1;
        break;
    }
    return rc;
}

static int _pepper_start_handler(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    int rc = 0;

    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        LOG_INFO("[pepper] gatt: start ...\n");
        uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
        if (om_len != sizeof(pepper_start_params_t)) {
            LOG_INFO("\terror, invalid parameters\n");
            rc = 1;
        }
        else {
            pepper_start_params_t params;
            rc = ble_hs_mbuf_to_flat(ctxt->om, &params, sizeof(params), &om_len);
            LOG_INFO("\tepoch_duration: %" PRIu32 " [s]\n", params.epoch_duration_s);
            if (params.epoch_iterations != 0 && params.epoch_iterations != UINT32_MAX) {
                LOG_INFO("\tepoch_iterations: %" PRIu32 "\n", params.epoch_iterations);
            }
            else {
                LOG_INFO("\tepoch_iterations: until stopped\n");
            }
            LOG_INFO("\tadv_per_slice: %" PRIu32 "\n", params.advs_per_slice);
            LOG_INFO("\tadv_itvl: %" PRIu32 " [ms]\n", params.adv_itvl_ms);
            LOG_INFO("\tscan_itvl: %" PRIu32 " [ms]\n", params.scan_itvl_ms);
            LOG_INFO("\tscan_win: %" PRIu32 " [ms]\n", params.scan_win_ms);
            pepper_start(&params);
        }
        break;

    case BLE_GATT_ACCESS_OP_READ_CHR:
        LOG_INFO("[pepper] gatt: start read characteristic\n");
        break;

    case BLE_GATT_ACCESS_OP_READ_DSC:
        LOG_INFO("[pepper] gatt: start read from descriptor\n");
        break;

    case BLE_GATT_ACCESS_OP_WRITE_DSC:
        LOG_INFO("[pepper] gatt: start write to descriptor\n");
        break;

    default:
        LOG_INFO("[pepper] gatt: start unhandled operation!\n");
        rc = 1;
        break;
    }

    return rc;
}

static int _pepper_stop_handler(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    int rc = 0;

    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        LOG_INFO("[pepper] gatt: stop ... ");
        pepper_stop();
        LOG_INFO("stopped\n");
        break;

    case BLE_GATT_ACCESS_OP_READ_CHR:
        LOG_INFO("[pepper] gatt: start read characteristic\n");
        break;

    case BLE_GATT_ACCESS_OP_READ_DSC:
        LOG_INFO("[pepper] gatt: start read from descriptor\n");
        break;

    case BLE_GATT_ACCESS_OP_WRITE_DSC:
        LOG_INFO("[pepper] gatt: start write to descriptor\n");
        break;

    default:
        LOG_INFO("[pepper] gatt: start unhandled operation!\n");
        rc = 1;
        break;
    }

    return rc;
}


static int _pepper_restart_handler(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    int rc = 0;

    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        LOG_INFO("[pepper] gatt: restart ... ");
        /* start pepper with default parameters */
        pepper_start_params_t params = {
            .epoch_duration_s = CONFIG_EPOCH_DURATION_SEC,
            .epoch_iterations = 0,
            .adv_itvl_ms = CONFIG_BLE_ADV_ITVL_MS,
            .advs_per_slice = CONFIG_ADV_PER_SLICE,
            .scan_itvl_ms = CONFIG_BLE_SCAN_ITVL_MS,
            .scan_win_ms = CONFIG_BLE_SCAN_WIN_MS,
            .align = false,
        };
        pepper_start(&params);
        break;

    case BLE_GATT_ACCESS_OP_READ_CHR:
        LOG_INFO("[pepper] gatt: start read characteristic\n");
        break;

    case BLE_GATT_ACCESS_OP_READ_DSC:
        LOG_INFO("[pepper] gatt: start read from descriptor\n");
        break;

    case BLE_GATT_ACCESS_OP_WRITE_DSC:
        LOG_INFO("[pepper] gatt: start write to descriptor\n");
        break;

    default:
        LOG_INFO("[pepper] gatt: start unhandled operation!\n");
        rc = 1;
        break;
    }

    return rc;
}

#if IS_USED(MODULE_PEPPER_CURRENT_TIME)
static int _pepper_current_time_handler(uint16_t conn_handle, uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    int rc = 0;

    switch (ctxt->op) {

    case BLE_GATT_ACCESS_OP_READ_CHR:
        LOG_INFO("[pepper] gatt: epoch %" PRIu32 "\n", ztimer_now(ZTIMER_EPOCH));
        uint8_t sys_epoch[sizeof(current_time_t)];
        byteorder_htolebufl(sys_epoch, ztimer_now(ZTIMER_EPOCH));
        rc = os_mbuf_append(ctxt->om, sys_epoch, sizeof(current_time_t));
        break;
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        LOG_INFO("[pepper] gatt: cfg write new epoch...\n");
        uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
        if (om_len > sizeof(current_time_t)) {
            LOG_INFO("error, exceeded buffer size\n");
            rc = 1;
        }
        else {
            uint8_t sys_epoch[sizeof(current_time_t)];
            rc = ble_hs_mbuf_to_flat(ctxt->om, sys_epoch,
                                     sizeof(uint32_t),
                                     &om_len);
            current_time_update(byteorder_lebuftohl(sys_epoch));
        }
        break;
    case BLE_GATT_ACCESS_OP_READ_DSC:
        LOG_DEBUG("[pepper] gatt: cfg read from descriptor\n");
        break;
    case BLE_GATT_ACCESS_OP_WRITE_DSC:
        LOG_DEBUG("[pepper] gatt: cfg write to descriptor\n");
        break;
    default:
        LOG_WARNING("[pepper] gatt: cfg unhandled operation!\n");
        rc = 1;
        break;
    }
    return rc;
}
#endif

void pepper_gatt_init(void)
{
    int rc = 0;

    char *uid = pepper_get_uid_str();

    /* verify and add our custom services */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    assert(rc == 0);
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    assert(rc == 0);

    /* set the device name */
    ble_svc_gap_device_name_set(uid);
    /* reload the GATT server to link our added services */
    ble_gatts_start();

    /* start to advertise this node with its name */
    nimble_autoadv_add_field(BLE_GAP_AD_NAME, uid, strlen(uid));
    nimble_autoadv_start(NULL);

    /* fix compilation error when using DEVELHELP=0 */
    (void)rc;
}
