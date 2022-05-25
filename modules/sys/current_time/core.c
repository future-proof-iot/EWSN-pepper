/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_current_time_client
 * @{
 *
 * @file
 * @brief       BLE Time Service Client implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <string.h>
#include <stdbool.h>

#include "irq.h"
#include "clist.h"
#include "ztimer.h"
#include "current_time.h"
#include "periph/gpio.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

static current_time_hook_t _pre;
static current_time_hook_t _post;
static bool _sync = false;

static void _toggle_sync_led(void)
{
    if (IS_USED(MODULE_CURRENT_TIME_LED)) {
        if (gpio_is_valid(CONFIG_CURRENT_TIME_SYNC_LED)) {
            gpio_toggle(CONFIG_CURRENT_TIME_SYNC_LED);
        }
    }
}

static bool _time_is_in_range(int32_t diff, int32_t range)
{
    return diff > 0 ? diff < range : -diff < range;
}

static int _hook_cb(clist_node_t *node, void *arg)
{
    current_time_hook_t *hook = (current_time_hook_t *)node;
    int32_t offset = *((uint32_t *)arg);

    hook->cb(offset, hook->arg);
    return 0;
}

uint32_t current_time_get(void)
{
    return ztimer_now(ZTIMER_EPOCH);
}

void current_time_update(uint32_t epoch)
{
    uint32_t sys_epoch = ztimer_now(ZTIMER_EPOCH);

    LOG_DEBUG("[current_time]: epoch\n");
    LOG_DEBUG("\tcurrent:     %" PRIu32 "\n", sys_epoch);
    LOG_DEBUG("\treceived:    %" PRIu32 "\n", epoch);
    int32_t diff = epoch - sys_epoch;

    /* adjust time only if out of CONFIG_CURRENT_TIME_RANGE_S */
    if (!_time_is_in_range(diff, CONFIG_CURRENT_TIME_RANGE_S)) {
        _toggle_sync_led();
        _sync = false;
        clist_foreach(&_pre.list_node, _hook_cb, &diff);
        uint32_t elapsed = ztimer_now(ZTIMER_EPOCH) - sys_epoch;
        ztimer_adjust_time(ZTIMER_EPOCH, diff + elapsed);
        LOG_DEBUG("\tnew-current: %" PRIu32 "\n", ztimer_now(ZTIMER_EPOCH));
        clist_foreach(&_post.list_node, _hook_cb, &diff);
        _sync = true;
        _toggle_sync_led();
    }
}

void current_time_add_pre_cb(current_time_hook_t *hook)
{
    unsigned state = irq_disable();

    if (!hook->list_node.next) {
        clist_rpush(&_pre.list_node, &hook->list_node);
    }
    irq_restore(state);
}

void current_time_rmv_pre_cb(current_time_hook_t *hook)
{
    unsigned state = irq_disable();

    clist_remove(&_pre.list_node, &hook->list_node);
    hook->list_node.next = NULL;
    irq_restore(state);
}

void current_time_add_post_cb(current_time_hook_t *hook)
{
    unsigned state = irq_disable();

    if (!hook->list_node.next) {
        clist_rpush(&_post.list_node, &hook->list_node);
    }
    irq_restore(state);
}

void current_time_rmv_post_cb(current_time_hook_t *hook)
{
    unsigned state = irq_disable();

    clist_remove(&_post.list_node, &hook->list_node);
    hook->list_node.next = NULL;
    irq_restore(state);
}

void current_time_init(void)
{
    memset(&_pre, '\0', sizeof(current_time_hook_t));
    memset(&_post, '\0', sizeof(current_time_hook_t));
    _sync = false;

    if (IS_USED(MODULE_CURRENT_TIME_BLE_SCANNER)) {
        current_time_init_ble_scanner();
    }
}

bool current_time_valid(void)
{
    return _sync;
}
