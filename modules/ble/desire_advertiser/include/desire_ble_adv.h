/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_desire_ble_adv EBID advertisment manager
 * @ingroup     sys
 * @brief       Desire BLE EBID advertisement in Carousel mode.
 *
 *
 * @{
 *
 * @file
 *
 * @author      Roudy Dagher <roudy.dagher@inria.fr>
 */

#ifndef DESIRE_BLE_ADV_H
#define DESIRE_BLE_ADV_H

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "timex.h"
#include "event.h"
#if IS_USED(MODULE_DESIRE_ADVERTISER_THREADED)
#include "event/thread.h"
#endif

#include "ebid.h"
#include "desire/ble_pkt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   The preffered advertisement TX power
 */
#ifndef CONFIG_BLE_ADV_TX_POWER
#define CONFIG_BLE_ADV_TX_POWER         (127)
#endif

/**
 * @brief   The Advertiser Instance
 */
#ifndef CONFIG_DESIRE_ADV_INST
#define CONFIG_DESIRE_ADV_INST          (0)
#endif

/**
 * @brief   The advertisement interval in milliseconds
 */
#ifndef CONFIG_BLE_ADV_ITVL_MS
#define CONFIG_BLE_ADV_ITVL_MS      (1LU * MS_PER_SEC)
#endif

/**
 * @brief   The static CID value
 */
#ifndef CONFIG_BLE_ADV_STATIC_CID
#define CONFIG_BLE_ADV_STATIC_CID       0
#endif

#if IS_USED(MODULE_DESIRE_ADVERTISER_THREADED)
/**
 * @brief   The advertisement thread stacksize
 */
#ifndef CONFIG_BLE_ADV_THREAD_STACKSIZE
#define CONFIG_BLE_ADV_THREAD_STACKSIZE (THREAD_STACKSIZE_DEFAULT)
#endif

/**
 * @brief   The advertisement thread priority
 */
#ifndef CONFIG_BLE_ADV_THREAD_PRIO
#define CONFIG_BLE_ADV_THREAD_PRIO       (THREAD_PRIORITY_MAIN - 4)
#endif
#endif

/**
 * @brief   The advertisement thread priority
 */
#ifndef CONFIG_BLE_ADV_SHELL_BLOCKING
#define CONFIG_BLE_ADV_SHELL_BLOCKING     0
#endif

/**
 * @brief   Advertise forever or until stopped manually
 */
#define BLE_ADV_TIMEOUT_NEVER           (UINT32_MAX)

/**
 * @brief   Advertising parameters structure
 */
typedef struct __attribute__((__packed__)) {
    uint32_t itvl_ms;       /**< the advertisement interval in milliseconds */
    int32_t advs_max;       /**< the amount of advertisement events */
    int32_t advs_slice;     /**< the amount of advertisements by slice before rotation */
} adv_params_t;

/**
 * @brief   Initialize the advertising module
 *
 * @param[in]   queue   The already initialized event queue to handle
 *                      advertisement events
 */
void desire_ble_adv_init(event_queue_t *queue);

#if IS_USED(MODULE_DESIRE_ADVERTISER_THREADED)
/**
 * @brief   Initialize the advertising module internal structure and advertising
 *          thread.
 */
void desire_ble_adv_init_threaded(void);
#endif

/**
 * @brief   Starts advertising an EBID following Desire Carousel scheme.
 *
 * The EBID is split in 3 slices with an extra XOR slice. Each slice is advertised
 * every 'itvl_ms' milliseconds 'advs_slice' times.
 *
 * @note If called while an advertisement procedure is ongoing it will stop the later
 *
 * @param[in]       ebid         the EBID to advertise
 */
void desire_ble_adv_start(ebid_t *ebid, adv_params_t *params);

/**
 * @brief   Stops current advertisements if any
 */
void desire_ble_adv_stop(void);

/**
 * @brief   Callback signature triggered after each advertisement
 *
 * @param[in] advs      current advertisement count since @desire_ble_adv_start
 * @param[in] arg       optional user set argument, can be NULL
 */
typedef void (*ble_adv_cb_t)(uint32_t advs, void *arg);

/**
 * @brief   Returns the current cid
 *
 * @return  the cid
 */
uint32_t desire_ble_adv_get_cid(void);

/**
 * @brief   Generates a valid cid
 *
 * @return  the cid
 */
uint32_t desire_ble_adv_gen_cid(void);

/**
 * @brief   Sets a new cid
 *
 * @note    If and advertisement procedure is ongoing this will only get
 *          updated on the next slice rotation, should really only be used
 *          for testing when CONFIG_BLE_ADV_STATIC_CID=1
 *
 * @param[in] cid       the cid to set
 */
void desire_ble_adv_set_cid(uint32_t cid);

/**
 * @brief   Sets a argument for callback called after every advertisement
 *
 * @param[in] arg       the callback argument
 */

void desire_ble_adv_set_cb_arg(void *arg);

/**
 * @brief   Sets a callback  to be called after every advertisement
 *
 * @param[in] cb        the callback
 */
void desire_ble_adv_set_cb(ble_adv_cb_t cb);

/**
 * @brief   Prints status information of the advertiser good to call
 *          on the adv callback
 */
void desire_ble_adv_status_print(void);

#ifdef __cplusplus
}
#endif

#endif /* DESIRE_BLE_ADV_H */
/** @} */
