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
 * @brief       PEPPER epoch synchronization service
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include "pepper.h"
#include "current_time.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

#ifndef CONFIG_EPOCH_MAX_TIME_OFFSET
#define CONFIG_EPOCH_MAX_TIME_OFFSET            (CONFIG_EPOCH_DURATION_SEC / 10)
#endif

static void _pre_cb(int32_t offset, void *arg);
static void _post_cb(int32_t offset, void *arg);
static current_time_hook_t _pre_hook = CURRENT_TIME_HOOK_INIT(_pre_cb, NULL);
static current_time_hook_t _post_hook = CURRENT_TIME_HOOK_INIT(_post_cb, NULL);

static bool _time_is_in_range(int32_t diff, int32_t range)
{
    return diff > 0 ? diff < range : -diff < range;
}

static void _pre_cb(int32_t offset, void *arg)
{
    (void)arg;
    if (!_time_is_in_range(offset, (int32_t)CONFIG_EPOCH_MAX_TIME_OFFSET)) {
        LOG_WARNING("[pepper] current_time: pause, time diff is too high\n");
        pepper_pause();
    }
}

static void _post_cb(int32_t offset, void *arg)
{
    (void)offset;
    (void)arg;
    if (!_time_is_in_range(offset, (int32_t)CONFIG_EPOCH_MAX_TIME_OFFSET)) {
        LOG_WARNING("[pepper] current_time: resume\n");
        pepper_resume(true);
    }
}

void pepper_current_time_init(void)
{
    current_time_init();
    current_time_add_pre_cb(&_pre_hook);
    current_time_add_post_cb(&_post_hook);
}
