/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_epoch
 * @{
 *
 * @file
 * @brief       Epoch Encounters implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <string.h>
#include <assert.h>

#include "irq.h"
#include "test_utils/result_output.h"

#include "epoch.h"
#include "ed.h"

typedef struct top_ed {
    ed_t *ed;
    uint16_t duration;
} top_ed_t;

typedef struct top_ed_list {
    top_ed_t top[CONFIG_EPOCH_MAX_ENCOUNTERS];
    uint8_t min;
    uint8_t count;
} top_ed_list_t;

static uint16_t ed_max_exposure_time(ed_t *ed)
{
    uint16_t exposure = 0;
    uint16_t tmp_exposure = 0;
    (void)tmp_exposure;
    (void)ed;

#if IS_USED(MODULE_ED_BLE)
    tmp_exposure = ed->ble.seen_last_s - ed->ble.seen_first_s;
    exposure = tmp_exposure > exposure ? tmp_exposure : exposure;
#endif
#if IS_USED(MODULE_ED_BLE_WIN)
    tmp_exposure = ed->ble_win.seen_last_s - ed->ble_win.seen_first_s;
    exposure = tmp_exposure > exposure ? tmp_exposure : exposure;
#endif
#if IS_USED(MODULE_ED_UWB)
    tmp_exposure = ed->uwb.seen_last_s - ed->uwb.seen_first_s;
    exposure = tmp_exposure > exposure ? tmp_exposure : exposure;
#endif
    return exposure;
}

static int _add_to_top_list(clist_node_t *node, void *arg)
{
    top_ed_list_t *list = (top_ed_list_t *)arg;
    uint16_t duration = ed_max_exposure_time((ed_t *)node);

    /* if still space in list simply insert */
    if (list->count < 8) {
        /* update the minimum value index if needed */
        if (list->top[list->min].duration > duration) {
            list->min = list->count;
        }
        list->top[list->count].ed = (ed_t *)node;
        list->top[list->count].duration = duration;
        list->count++;
    }
    else {
        /* if exposure is smaller than the minimum exposure in the list then
           ignore it */
        if (duration <= list->top[list->min].duration) {
            return 0;
        }
        else {
            /* replace the current minimum by the new encounter */
            list->top[list->min].ed = (ed_t *)node;
            list->top[list->min].duration = duration;
            /* update the minimum exposure time */
            for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
                if (list->top[i].duration < list->top[list->min].duration) {
                    list->min = i;
                }
            }
        }
    }
    return 0;
}

void epoch_init(epoch_data_t *epoch, uint32_t timestamp,
                crypto_manager_keys_t *keys)
{
    memset(epoch, '\0', sizeof(epoch_data_t));
    epoch->keys = keys;
    epoch->timestamp = timestamp;
    if (keys) {
        crypto_manager_gen_keypair(keys);
    }
}

void epoch_finish(epoch_data_t *epoch, ed_list_t *list)
{
    /* process all data */
    top_ed_list_t top;

    memset(&top, '\0', sizeof(top));

    /* finish list processing */
    clist_foreach(&list->list, _add_to_top_list, &top);
    for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
        if (top.top[i].duration != 0) {
#if IS_USED(MODULE_ED_UWB)
            epoch->contacts[i].uwb.exposure_s = top.top[i].ed->uwb.seen_last_s -
                                                top.top[i].ed->uwb.seen_first_s;
            epoch->contacts[i].uwb.avg_d_cm = top.top[i].ed->uwb.cumulative_d_cm;
#if IS_USED(MODULE_ED_UWB_LOS)
            epoch->contacts[i].uwb.avg_los = top.top[i].ed->uwb.cumulative_los;
#endif
            epoch->contacts[i].uwb.req_count = top.top[i].ed->uwb.req_count;

#if IS_USED(MODULE_ED_UWB_STATS)
            memcpy(&epoch->contacts[i].uwb.stats, &top.top[i].ed->uwb.stats,
                   sizeof(ed_uwb_stats_t));
#endif
#endif
#if IS_USED(MODULE_ED_BLE)
            epoch->contacts[i].ble.exposure_s = top.top[i].ed->ble.seen_last_s -
                                                top.top[i].ed->ble.seen_first_s;
            epoch->contacts[i].ble.avg_rssi = top.top[i].ed->ble.cumulative_rssi;
            epoch->contacts[i].ble.avg_d_cm = top.top[i].ed->ble.cumulative_d_cm;
            epoch->contacts[i].ble.scan_count = top.top[i].ed->ble.scan_count;
#endif
#if IS_USED(MODULE_ED_BLE_WIN)
            epoch->contacts[i].ble_win.exposure_s = top.top[i].ed->ble_win.seen_last_s -
                                                    top.top[i].ed->ble_win.seen_first_s;
            memcpy(epoch->contacts[i].ble_win.wins, &top.top[i].ed->ble_win.wins,
                   sizeof(rdl_windows_t));
#endif
            crypto_manager_gen_pets(epoch->keys, top.top[i].ed->ebid.parts.ebid.u8,
                                    &epoch->contacts[i].pet);
        }
    }
    ed_list_clear(list);
}

bool epoch_valid_contact(contact_data_t *data)
{
    bool valid = false;
    (void)data;

#if IS_USED(MODULE_ED_BLE)
    valid |= (data->ble.exposure_s != 0);
#endif
#if IS_USED(MODULE_ED_BLE_WIN)
    valid |= (data->ble_win.exposure_s != 0);
#endif
#if IS_USED(MODULE_ED_UWB)
    valid |= (data->uwb.exposure_s != 0);
#endif
    return valid;
}

uint8_t epoch_contacts(epoch_data_t *epoch)
{
    uint8_t contacts = 0;

    for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
        if (epoch_valid_contact(&epoch->contacts[i])) {
            contacts++;
        }
    }
    return contacts;
}

void epoch_data_memory_manager_init(epoch_data_memory_manager_t *manager)
{
    memset(manager, '\0', sizeof(epoch_data_memory_manager_t));
    memarray_init(&manager->mem, manager->buf, sizeof(epoch_data_t), CONFIG_EPOCH_DATA_BUF_SIZE);
}

void epoch_data_memory_manager_free(epoch_data_memory_manager_t *manager,
                                    epoch_data_t *epoch_data)
{
    memarray_free(&manager->mem, epoch_data);
}

epoch_data_t *epoch_data_memory_manager_calloc(epoch_data_memory_manager_t *manager)
{
    return memarray_calloc(&manager->mem);
}
