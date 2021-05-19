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

#include "epoch.h"
#include "fmt.h"
#include "log.h"
#include "base64.h"
#include "test_utils/result_output.h"

typedef struct top_ed {
    ed_t *ed;
    uint16_t duration;
} top_ed_t;

typedef struct top_ed_list {
    top_ed_t top[CONFIG_EPOCH_MAX_ENCOUNTERS];
    uint8_t min;
    uint8_t count;
} top_ed_list_t;

/* TODO: this is a ver bad search algorithm, improve later */
static int _add_to_top_list(clist_node_t *node, void *arg)
{
    top_ed_list_t *list = (top_ed_list_t *)arg;
    uint16_t duration = ed_exposure_time((ed_t *)node);

    if (list->count < 8) {
        if (list->top[list->min].duration > duration) {
            list->min = list->count;
        }
        list->top[list->count].ed = (ed_t *)node;
        list->top[list->count].duration = duration;
        list->count++;
    }
    else {
        if (duration <= list->top[list->min].duration) {
            return 0;
        }
        else {
            list->top[list->min].ed = (ed_t *)node;
            list->top[list->min].duration = duration;
            for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
                if (list->top[i].duration < list->top[list->min].duration) {
                    list->min = i;
                }
            }
        }
    }
    return 0;
}

void epoch_init(epoch_data_t *epoch, uint16_t timestamp,
                              crypto_manager_keys_t* keys)
{
    memset(epoch, '\0', sizeof(epoch_data_t));
    epoch->keys = keys;
    epoch->timestamp = timestamp;
    crypto_manager_gen_keypair(keys);
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
            epoch->contacts[i].duration = top.top[i].duration;
            memcpy(epoch->contacts[i].wins, &top.top[i].ed->wins,
                   sizeof(rdl_windows_t));
            crypto_manager_gen_pets(epoch->keys,
                                    top.top[i].ed->ebid.parts.ebid.u8,
                                    &epoch->contacts[i].pet);
            epoch->contacts[i].obf = top.top[i].ed->obf;
        }
    }

    while (list->list.next) {
        ed_memory_manager_free(list->manager, (ed_t *)clist_lpop(&list->list));
    }
}

void epoch_serialize_printf(epoch_data_t *epoch)
{
    turo_t ctx;

    turo_init(&ctx);
    turo_dict_open(&ctx);
    turo_dict_key(&ctx, "epoch");
    turo_u32(&ctx, epoch->timestamp);
    turo_dict_key(&ctx, "pets");
    turo_array_open(&ctx);
    for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
        if (epoch->contacts[i].duration != 0) {
            turo_dict_open(&ctx);
            turo_dict_key(&ctx, "pet");
            turo_array_open(&ctx);
            uint8_t b64_buff[64];
            size_t b64_len = sizeof(b64_buff);
            base64_encode(epoch->contacts[i].pet.et, PET_SIZE, b64_buff,
                          &b64_len);
            turo_string(&ctx, (char *)b64_buff);
            base64_encode(epoch->contacts[i].pet.rt, PET_SIZE, b64_buff,
                          &b64_len);
            turo_string(&ctx, (char *)b64_buff);
            turo_array_close(&ctx);
            turo_dict_key(&ctx, "duration");
            turo_u32(&ctx, epoch->contacts[i].duration);
            turo_dict_key(&ctx, "Gtx");
            turo_u32(&ctx, CONFIG_TX_COMPENSATION_GAIN - epoch->contacts[i].obf);
            turo_dict_key(&ctx, "windows");
            turo_array_open(&ctx);
            for (uint8_t j = 0; j < WINDOWS_PER_EPOCH; j++) {
                turo_dict_open(&ctx);
                turo_dict_key(&ctx, "samples");
                turo_u32(&ctx, epoch->contacts[i].wins[j].samples);
                turo_dict_key(&ctx, "rssi");
                turo_float(&ctx, epoch->contacts[i].wins[j].avg);
                turo_dict_close(&ctx);
            }
            turo_array_close(&ctx);
            turo_dict_close(&ctx);
        }
    }
    turo_array_close(&ctx);
    turo_dict_close(&ctx);
    print_str("\n");
}
