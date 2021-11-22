/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_current_time BLE Time Service Module
 * @ingroup     sys
 * @brief       A time synchronization BLE service
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <femolina@uc.cl>
 */

#ifndef CURRENT_TIME_H
#define CURRENT_TIME_H

#include <string.h>
#include <inttypes.h>
#include "clist.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CURRENT_TIME_SERVICE_UUID16         0x3333

/**
 * @brief   Acceptable offset between current and received time in seconds
 *
 */
#ifndef CONFIG_CURRENT_TIME_RANGE_S
#define CONFIG_CURRENT_TIME_RANGE_S         (5U)
#endif

/* TODO: this could probably be an event */

/**
 * @brief   Time service callback type
 *
 * @param[in]   offset  the time correction offset
 * @param[in]   arg     optional user arg
 */
typedef void (*current_time_cb_t)(int32_t, void *);

typedef union __attribute__((packed)) current_time {
    uint32_t epoch;
    uint8_t bytes[4];
} current_time_t;

/**
 * @brief    Time service adjusted time hooks
 */
typedef struct current_time_hooks {
    clist_node_t list_node;     /**< the list node item */
    current_time_cb_t cb;       /**< the callback */
    void *arg;                  /**< optional arg */
} current_time_hook_t;

/**
 * @brief   Time service init, register the callback with desire_ble_scannner
 */
void current_time_init(void);

/**
 * @brief   Add a callback to be called before time is adjusted
 *
 * @param[in]   hook    the hook
 */
void current_time_add_pre_cb(current_time_hook_t *hook);

/**
 * @brief   Remove a callback to be called before time is adjusted
 *
 * @param[in]   hook    the hook
 */
void current_time_rmv_pre_cb(current_time_hook_t *hook);

/**
 * @brief   Remove a callback to be called after time is adjusted
 *
 * @param[in]   hook    the hook
 */
void current_time_add_post_cb(current_time_hook_t *hook);

/**
 * @brief   Add a callback to be called after time is adjusted
 *
 * @param[in]   hook    the hook
 */
void current_time_rmv_post_cb(current_time_hook_t *hook);

/**
 * @brief   Utility to init a current_time_hook
 * @param[inout]    hook    the hook to init
 * @param[in]       cb      the callback
 * @param[in]       arg     the optional callback argument
 */
static inline void current_time_hook_init(current_time_hook_t *hook,
                                          current_time_cb_t cb, void *arg)
{
    memset(hook, '\0', sizeof(current_time_hook_t));
    hook->cb = cb;
    hook->arg = arg;
}

/**
 * @brief   Current Time Hook Static Initialized
 *
 * @param[in]       _cb      the callback
 * @param[in]       _arg     the optional callback argument
 */
#define CURRENT_TIME_HOOK_INIT(_cb, _arg) \
    { \
        .list_node.next = NULL, \
        .cb = _cb, \
        .arg = (void *)_arg \
    }

/**
 * @brief   Update current time
 *
 * @param[in]       time   epoch in seconds since RIOT_EPOCH
 */
void current_time_update(uint32_t time);

#ifdef __cplusplus
}
#endif

#endif /* CURRENT_TIME_H */
/** @} */
