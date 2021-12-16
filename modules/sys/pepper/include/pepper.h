/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_pepper PrEcise Privacy-PresERving Proximity Tracing (PEPPER)
 * @ingroup     sys
 * @brief       PrEcise Privacy-PresERving Proximity Tracing module
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef PEPPER_H
#define PEPPER_H

#include "event/thread.h"
#include "ztimer/config.h"
#include "timex.h"
#include "pepper/config.h"
#include "epoch.h"
#include "ed.h"
#if IS_USED(MODULE_TWR)
#include "twr.h"
#endif
#include "ebid.h"
#if IS_USED(MODULE_DESIRE_ADVERTISER)
#include "desire_ble_adv.h"
#include "desire_ble_scan.h"
#include "desire_ble_scan_params.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   The following event priorities are defined in a way that:
 *              - epoch state machine event come before any other events
 *              - uwb & ble events have same priority
 *              - serialization events are handled last
 *
 *          This allows a single even thread to handle all events.
 */
#ifndef CONFIG_PEPPER_EVENT_PRIO
#define CONFIG_PEPPER_EVENT_PRIO        (EVENT_PRIO_HIGHEST)
#endif

#ifndef CONFIG_PEPPER_LOW_EVENT_PRIO
#define CONFIG_PEPPER_LOW_EVENT_PRIO    (EVENT_PRIO_LOWEST)
#endif

#ifndef CONFIG_UWB_BLE_EVENT_PRIO
#define CONFIG_UWB_BLE_EVENT_PRIO       (EVENT_PRIO_MEDIUM)
#endif

#ifndef CONFIG_TWR_MIN_OFFSET_MS
#define CONFIG_TWR_MIN_OFFSET_MS        (100LU)
#endif

#ifndef CONFIG_TWR_MIN_OFFSET_TICKS
#define CONFIG_TWR_MIN_OFFSET_TICKS     ((CONFIG_TWR_MIN_OFFSET_MS) * (CONFIG_ZTIMER_MSEC_BASE_FREQ / MS_PER_SEC))
#endif

#ifndef CONFIG_TWR_RX_OFFSET_TICKS
#define CONFIG_TWR_RX_OFFSET_TICKS      (-20)
#endif

#ifndef CONFIG_TWR_TX_OFFSET_TICKS
#define CONFIG_TWR_TX_OFFSET_TICKS      (0)
#endif

#ifndef CONFIG_MIA_TIME_S
#define CONFIG_MIA_TIME_S               (30LU)
#endif

#ifndef CONFIG_PEPPER_BASE_NAME_BUFFER
#define CONFIG_PEPPER_BASE_NAME_BUFFER  (32)
#endif
/**
 * @brief   If active shell `pepper_start` commands will block until
 *          completion
 *
 */
#ifndef CONFIG_PEPPER_SHELL_BLOCKING
#define CONFIG_PEPPER_SHELL_BLOCKING    0
#endif

#ifndef CONFIG_PEPPER_LOGFILE
#define CONFIG_PEPPER_LOGFILE           "sys/log/pepper.txt"
#endif

#ifndef CONFIG_PEPPER_LOG_BLE
#define CONFIG_PEPPER_LOG_BLE       0
#endif

#ifndef CONFIG_PEPPER_LOG_UWB
#define CONFIG_PEPPER_LOG_UWB       0
#endif



typedef struct twr_params {
    int16_t rx_offset_ticks;
    int16_t tx_offset_ticks;
} twr_params_t;

typedef struct __attribute__((__packed__)) {
    uint32_t duration_s;
    uint32_t iterations;
} epoch_params_t;

typedef struct __attribute__((__packed__)) {
    uint32_t epoch_duration_s;
    uint32_t epoch_iterations;
    uint32_t advs_per_slice;
    uint32_t adv_itvl_ms;
    bool align;
} pepper_start_params_t;

typedef enum {
    PEPPER_STOPPED,
    PEPPER_PAUSED,
    PEPPER_RUNNING,
} pepper_status_t;

typedef struct controller {
    ebid_t ebid;                        /**> */
    ed_list_t ed_list;                  /**> */
    ed_memory_manager_t ed_mem;         /**> */
#if IS_USED(MODULE_TWR)
    twr_event_mem_manager_t twr_mem;    /**> */
    twr_params_t twr_params;            /**> */
#endif
    crypto_manager_keys_t keys;         /**> */
    uint32_t start_time;                /**> */
    epoch_data_t data;                  /**> */
    epoch_data_t data_serialize;        /**> */
    mutex_t lock;                       /**> */
    epoch_params_t epoch;               /**> */
#if IS_USED(MODULE_DESIRE_ADVERTISER)
    adv_params_t adv;
#endif
    pepper_status_t status;             /**> */
} controller_t;

void pepper_init(void);
void pepper_start(pepper_start_params_t * params);
void pepper_stop(void);
void pepper_pause(void);
void pepper_resume(bool align);
bool pepper_is_active(void);
/* internal use only */
controller_t *pepper_get_controller(void);

void pepper_twr_set_rx_offset(int16_t ticks);
void pepper_twr_set_tx_offset(int16_t ticks);
int16_t pepper_twr_get_rx_offset(void);
int16_t pepper_twr_get_tx_offset(void);

#if IS_USED(MODULE_PEPPER_UTIL)
int pepper_set_serializer_bn(char* base_name);
char* pepper_get_serializer_bn(void);
#else
static inline int pepper_set_serializer_bn(char* base_name)
{
    (void)base_name;
    return -1;
}
static inline char* pepper_get_serializer_bn(void)
{
    return NULL;
}
#endif
char *pepper_get_uid(void);

void pepper_uid_init(void);
void pepper_gatt_init(void);
void pepper_stdio_nimble_init(void);
void pepper_current_time_init(void);
#ifdef __cplusplus
}
#endif

#endif /* PEPPER_H */
/** @} */
