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
#include "log.h"
#include "periph/rtc.h"
#include "ztimer.h"

#if IS_USED(MODULE_DESIRE_SCANNER)
#include "desire_ble_scan.h"
#endif
#include "time_ble_pkt.h"
#include "current_time.h"

static current_time_hook_t _pre;
static current_time_hook_t _post;

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

static void _time_update_cb(const current_time_ble_adv_payload_t *time)
{
    /* parse advertisement payload to epoch timestamp*/
    struct tm t;

    current_time_ble_adv_parse(time, &t);
    uint32_t new_now = rtc_mktime(&t);

    uint32_t now = ztimer_now(ZTIMER_EPOCH);
    LOG_DEBUG("[current_time]: epoch\n");
    LOG_DEBUG("\tcurrent:     %" PRIu32 "\n", now);
    LOG_DEBUG("\treceived:    %" PRIu32 "\n", new_now);
    int32_t diff = new_now - now;
    /* adjust time only if out of CONFIG_CURRENT_TIME_RANGE_S */
    if (!_time_is_in_range(diff, CONFIG_CURRENT_TIME_RANGE_S)) {
        clist_foreach(&_pre.list_node, _hook_cb, &diff);
        LOG_DEBUG("\tnew-current: %" PRIu32 "\n",
                  (uint32_t)ztimer_now(ZTIMER_EPOCH));
        ztimer_adjust_time(ZTIMER_EPOCH, diff);
        clist_foreach(&_post.list_node, _hook_cb, &diff);
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
    /* set the update callback */
    desire_ble_set_time_update_cb(_time_update_cb);
}
