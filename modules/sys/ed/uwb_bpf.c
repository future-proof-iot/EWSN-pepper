
/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_ed
 * @{
 *
 * @file
 * @brief       Encounter Data List implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <assert.h>
#include <string.h>

#include "mutex.h"
#include "femtocontainer/femtocontainer.h"
#include "shared.h"
#if IS_USED(MODULE_UWB_BPF_SUIT)
#include "suit/transport/coap.h"
#include "suit/storage.h"
#include "suit/storage/ram.h"
#include "net/gcoap.h"
#endif

#include "ed.h"
#include "bpf/ed.h"

#include "blob/bpf/contact_filter.bin.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_DEBUG
#endif
#include "log.h"

static uint8_t _stack[512] = { 0 };
static mutex_t _lock = MUTEX_INIT;

#if IS_USED(MODULE_UWB_BPF_SUIT)
suit_storage_region_t *region = NULL;
static suit_storage_hooks_t pre;
static suit_storage_hooks_t post;

/* CoAP resources (alphabetical order) */
static const coap_resource_t _resources[] = {
    /* this line adds the whole "/suit"-subtree */
    SUIT_COAP_SUBTREE,
};
static gcoap_listener_t _suit_listener = {
    (coap_resource_t *)&_resources[0],
    sizeof(_resources) / sizeof(_resources[0]),
    NULL,
    NULL,
    NULL
};

static void _lock_region(void *arg)
{
    LOG_INFO("[femto-container]: updating, lock region\n");
    mutex_t *lock = (mutex_t *)arg;

    mutex_lock(lock);
}

static void _unlock_region(void *arg)
{
    LOG_INFO("[femto-container]: update finished, unlock region\n");
    mutex_t *lock = (mutex_t *)arg;

    mutex_unlock(lock);
}
#endif

void ed_uwb_bpf_init(void)
{
#if IS_USED(MODULE_UWB_BPF_SUIT)
    suit_storage_init_all();
    region = suit_storage_get_region_by_id(".ram.0");
    if (!region) {
        LOG_ERROR("[ed_bpf]: ERROR, did not find storage region");
        return;
    }
    /* initial bootstrapping maybe there should be an api for this.. */
    region->used = sizeof(contact_filter_bin);
    memcpy(suit_storage_region_location(region), contact_filter_bin, region->used);

    /* install hooks */
    pre.arg = &_lock;
    pre.cb = _lock_region;
    post.arg = &_lock;
    post.cb = _unlock_region;
    suit_storage_add_pre_hook(".ram.0", &pre);
    suit_storage_add_post_hook(".ram.0", &post);

    /* start suit coap updater thread */
    suit_coap_run();

    /* start gcoap listener */
    gcoap_register_listener(&_suit_listener);
#endif
}

bool ed_uwb_bpf_finish(ed_t *ed)
{
    if (ed->uwb.req_count > 0) {
        ed->uwb.cumulative_d_cm = ed->uwb.cumulative_d_cm / ed->uwb.req_count;
    }

    uint16_t time = ed->uwb.seen_last_s - ed->uwb.seen_first_s;
    ed_uwb_bpf_ctx_t ctx = {
        .time = time,
        .distance = ed->uwb.cumulative_d_cm,
        .req_count = ed->uwb.req_count
    };

    LOG_DEBUG("[ed_bpf]: time=(%" PRIu16 "), d=(%" PRIu32 "), req=(%" PRIu16 ")\n",
              time, ed->uwb.cumulative_d_cm, ed->uwb.req_count);

    f12r_t femtoc = {
#if IS_USED(MODULE_UWB_BPF_SUIT)
        .application = suit_storage_region_location(region),
        .application_len = suit_storage_region_size_used(region),
#else
        .application = contact_filter_bin,
        .application_len = sizeof(contact_filter_bin),
#endif
        .stack = _stack,
        .stack_size = sizeof(_stack),
    };

    int64_t result = 0;

    LOG_INFO("[encounter_record]: log... ");
    f12r_setup(&femtoc);
    mutex_lock(&_lock);
    f12r_execute_ctx(&femtoc, &ctx, sizeof(ctx), &result);
    mutex_unlock(&_lock);
    if (result == 1) {
        LOG_INFO("YES\n");
        ed->uwb.valid = true;
        return true;
    }
    LOG_INFO("NO\n");
    return false;
}
