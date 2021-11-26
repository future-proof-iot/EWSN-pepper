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

#include "ztimer/config.h"
#include "timex.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_TWR_MIN_OFFSET_MS
#define CONFIG_TWR_MIN_OFFSET_MS        (100)
#endif

#ifndef CONFIG_TWR_MIN_OFFSET_TICKS
#define CONFIG_TWR_MIN_OFFSET_TICKS     ((CONFIG_TWR_MIN_OFFSET_MS) * (CONFIG_ZTIMER_MSEC_BASE_FREQ / MS_PER_SEC))
#endif

#ifndef CONFIG_TWR_RX_OFFSET_TICKS
#define CONFIG_TWR_RX_OFFSET_TICKS      (0)
#endif

#ifndef CONFIG_TWR_TX_OFFSET_TICKS
#define CONFIG_TWR_TX_OFFSET_TICKS      (0)
#endif

#ifndef CONFIG_MIA_TIME_S
#define CONFIG_MIA_TIME_S               (5)
#endif

#ifndef CONFIG_PEPPER_BASE_NAME_BUFFER
#define CONFIG_PEPPER_BASE_NAME_BUFFER  (32)
#endif

#ifndef CONFIG_PEPPER_LOGFILE
#define CONFIG_PEPPER_LOGFILE           "sys/log/pepper.txt"
#endif

typedef struct twr_params {
    int16_t rx_offset_ticks;
    int16_t tx_offset_ticks;
} twr_params_t;

typedef struct adv_params {
    uint32_t itvl_ms;
    int32_t max_events;
    int32_t max_events_slice;
} adv_params_t;

typedef struct __attribute__((__packed__)) {
    uint32_t epoch_duration_s;
    uint32_t epoch_iterations;
    uint32_t advs_per_slice;
    uint32_t adv_itvl_ms;
    bool align;
} pepper_start_params_t;

void pepper_init(void);
void pepper_start(pepper_start_params_t * params);
void pepper_stop(void);
uint32_t pepper_pause(void);
void pepper_twr_set_rx_offset(int16_t ticks);
void pepper_twr_set_tx_offset(int16_t ticks);
int16_t pepper_twr_get_rx_offset(void);
int16_t pepper_twr_get_tx_offset(void);
int pepper_set_serializer_base_name(char* base_name);
char* pepper_get_base_name(void);
void pepper_current_time_init(void);
bool pepper_is_running(void);
char *pepper_get_uid(void);
void pepper_uid_init(void);
void pepper_gatt_init(void);
void pepper_stdio_nimble_init(void);
#ifdef __cplusplus
}
#endif

#endif /* PEPPER_H */
/** @} */
