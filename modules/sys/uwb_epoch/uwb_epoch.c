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
#include "uwb_epoch.h"
#include "uwb_ed.h"
#include "fmt.h"
#include "log.h"
#include "test_utils/result_output.h"

typedef struct top_uwb_ed {
    uwb_ed_t *ed;
    uint16_t duration;
} top_uwb_ed_t;

typedef struct top_uwb_ed_list {
    top_uwb_ed_t top[CONFIG_EPOCH_MAX_ENCOUNTERS];
    uint8_t min;
    uint8_t count;
} top_uwb_ed_list_t;

/* TODO: this is a ver bad search algorithm, improve later */
static int _add_to_top_list(clist_node_t *node, void *arg)
{
    top_uwb_ed_list_t *list = (top_uwb_ed_list_t *)arg;
    uint16_t duration = uwb_ed_exposure_time((uwb_ed_t *)node);

    if (list->count < 8) {
        if (list->top[list->min].duration > duration) {
            list->min = list->count;
        }
        list->top[list->count].ed = (uwb_ed_t *)node;
        list->top[list->count].duration = duration;
        list->count++;
    }
    else {
        if (duration <= list->top[list->min].duration) {
            return 0;
        }
        else {
            list->top[list->min].ed = (uwb_ed_t *)node;
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

void uwb_epoch_init(uwb_epoch_data_t *epoch, uint16_t timestamp,
                              crypto_manager_keys_t* keys)
{
    memset(epoch, '\0', sizeof(uwb_epoch_data_t));
    epoch->keys = keys;
    epoch->timestamp = timestamp;
    crypto_manager_gen_keypair(keys);
}

void uwb_epoch_finish(uwb_epoch_data_t *epoch, uwb_ed_list_t *list)
{
    /* process all data */
    top_uwb_ed_list_t top;
    memset(&top, '\0', sizeof(top));

    /* finish list processing */
    clist_foreach(&list->list, _add_to_top_list, &top);
    for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
        if (top.top[i].duration != 0) {
            epoch->contacts[i].exposure_s = top.top[i].duration;
            epoch->contacts[i].avg_d_cm = top.top[i].ed->cumulative_d_cm;
            epoch->contacts[i].req_count = top.top[i].ed->req_count;
            crypto_manager_gen_pets(epoch->keys,
                                    top.top[i].ed->ebid.parts.ebid.u8,
                                    &epoch->contacts[i].pet);
        }
    }

    while (list->list.next) {
        uwb_ed_memory_manager_free(list->manager, (uwb_ed_t *)clist_lpop(&list->list));
    }
}

static void turo_array_hex(turo_t *ctx, uint8_t *vals, size_t size)
{
    if (ctx->state == 1) {
        print_str(",");
    }
    ctx->state = 1;
    print_str("\"");
    while (size > 0) {
       print_byte_hex(*vals);
        vals++;
        size--;
    }
    print_str("\"");
}

void uwb_epoch_serialize_printf(uwb_epoch_data_t *epoch)
{
    turo_t ctx;

    unsigned int state = irq_disable();
    turo_init(&ctx);
    turo_dict_open(&ctx);
    turo_dict_key(&ctx, "epoch");
    turo_u32(&ctx, epoch->timestamp);
    turo_dict_key(&ctx, "pets");
    turo_array_open(&ctx);
    for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
        if (epoch->contacts[i].exposure_s != 0) {
            turo_dict_open(&ctx);
            turo_dict_key(&ctx, "pet");
            turo_dict_open(&ctx);
            turo_dict_key(&ctx, "etl");
            turo_array_hex(&ctx, epoch->contacts[i].pet.et, PET_SIZE);
            turo_dict_key(&ctx, "rtl");
            turo_array_hex(&ctx, epoch->contacts[i].pet.rt, PET_SIZE);
            turo_dict_close(&ctx);
            turo_dict_close(&ctx);
            turo_dict_open(&ctx);
            turo_dict_key(&ctx, "exposure");
            turo_u32(&ctx, epoch->contacts[i].exposure_s);
            turo_dict_close(&ctx);
            turo_dict_open(&ctx);
            turo_dict_key(&ctx, "req-count");
            turo_u32(&ctx, epoch->contacts[i].req_count);
            turo_dict_close(&ctx);
            turo_dict_open(&ctx);
            turo_dict_key(&ctx, "avg-d-cm");
            turo_u32(&ctx, epoch->contacts[i].avg_d_cm);
            turo_dict_close(&ctx);
       }
    }
    turo_array_close(&ctx);
    turo_dict_close(&ctx);
    print_str("\n");
    irq_restore(state);
}
