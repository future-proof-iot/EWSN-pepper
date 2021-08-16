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
#include "nanocbor/nanocbor.h"

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

void uwb_epoch_init(uwb_epoch_data_t *epoch, uint32_t timestamp,
                    crypto_manager_keys_t *keys)
{
    memset(epoch, '\0', sizeof(uwb_epoch_data_t));
    epoch->keys = keys;
    epoch->timestamp = timestamp;
    if (keys) {
        crypto_manager_gen_keypair(keys);
    }
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
        uwb_ed_memory_manager_free(list->manager,
                                   (uwb_ed_t *)clist_lpop(&list->list));
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
            turo_dict_key(&ctx, "exposure");
            turo_u32(&ctx, epoch->contacts[i].exposure_s);
            turo_dict_key(&ctx, "reqcount");
            turo_u32(&ctx, epoch->contacts[i].req_count);
            turo_dict_key(&ctx, "avg_d_cm");
            turo_u32(&ctx, epoch->contacts[i].avg_d_cm);
            turo_dict_close(&ctx);
            turo_dict_close(&ctx);
        }
    }
    turo_array_close(&ctx);
    turo_dict_close(&ctx);
    print_str("\n");
    irq_restore(state);
}

uint8_t uwb_epoch_contacts(uwb_epoch_data_t *epoch)
{
    uint8_t contacts = 0;

    for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
        if (epoch->contacts[i].exposure_s != 0) {
            contacts++;
        }
    }
    return contacts;
}

size_t uwb_epoch_serialize_cbor(uwb_epoch_data_t *epoch, uint8_t *buf,
                                size_t len)
{
    nanocbor_encoder_t enc;

    nanocbor_encoder_init(&enc, buf, len);
    nanocbor_fmt_tag(&enc, UWB_EPOCH_CBOR_TAG);
    nanocbor_fmt_array(&enc, 2);
    nanocbor_fmt_uint(&enc, epoch->timestamp);
    uint8_t contacts = uwb_epoch_contacts(epoch);
    nanocbor_fmt_array(&enc, contacts);
    for (uint8_t i = 0; i < contacts; i++) {
        nanocbor_fmt_array(&enc, 5);
        nanocbor_put_bstr(&enc, epoch->contacts[i].pet.et, PET_SIZE);
        nanocbor_put_bstr(&enc, epoch->contacts[i].pet.rt, PET_SIZE);
        nanocbor_fmt_uint(&enc, epoch->contacts[i].exposure_s);
        nanocbor_fmt_uint(&enc, epoch->contacts[i].req_count);
        nanocbor_fmt_uint(&enc, epoch->contacts[i].avg_d_cm);
    }
    return nanocbor_encoded_len(&enc);
}

int uwb_epoch_load_cbor(uint8_t *buf, size_t len, uwb_epoch_data_t *epoch)
{
    nanocbor_value_t dec;
    nanocbor_value_t arr1;
    nanocbor_value_t arr2;
    nanocbor_value_t arr3;
    uint32_t tag;

    nanocbor_decoder_init(&dec, buf, len);
    nanocbor_get_tag(&dec, &tag);
    if (tag != UWB_EPOCH_CBOR_TAG) {
        return -1;
    }
    nanocbor_enter_array(&dec, &arr1);
    if (!nanocbor_get_uint32(&arr1, &epoch->timestamp)) {
        return -1;
    }
    nanocbor_enter_array(&arr1, &arr2);
    for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
        if (nanocbor_at_end(&arr2)) {
            break;
        }
        nanocbor_enter_array(&arr2, &arr3);
        size_t blen;
        const uint8_t *pet;
        nanocbor_get_bstr(&arr3, &pet, &blen);
        memcpy(epoch->contacts[i].pet.et, pet, blen);
        nanocbor_get_bstr(&arr3, &pet, &blen);
        memcpy(epoch->contacts[i].pet.rt, pet, blen);
        nanocbor_get_uint16(&arr3, &epoch->contacts[i].exposure_s);
        nanocbor_get_uint16(&arr3, &epoch->contacts[i].req_count);
        nanocbor_get_uint8(&arr3, &epoch->contacts[i].avg_d_cm);
        nanocbor_leave_container(&arr2, &arr3);
    }

    nanocbor_leave_container(&arr1, &arr2);
    nanocbor_leave_container(&dec, &arr1);
    return 0;
}

size_t uwb_contact_serialize_cbor(uwb_contact_data_t *contact,
                                  uint32_t timestamp,
                                  uint8_t *buf, size_t len)
{
    nanocbor_encoder_t enc;

    nanocbor_encoder_init(&enc, buf, len);
    nanocbor_fmt_tag(&enc, UWB_EPOCH_CBOR_TAG);
    nanocbor_fmt_array(&enc, 6);
    nanocbor_fmt_uint(&enc, timestamp);
    nanocbor_put_bstr(&enc, contact->pet.et, PET_SIZE);
    nanocbor_put_bstr(&enc, contact->pet.rt, PET_SIZE);
    nanocbor_fmt_uint(&enc, contact->exposure_s);
    nanocbor_fmt_uint(&enc, contact->req_count);
    nanocbor_fmt_uint(&enc, contact->avg_d_cm);
    return nanocbor_encoded_len(&enc);
}

int uwb_contact_load_cbor(uint8_t *buf, size_t len,
                          uwb_contact_data_t *contact, uint32_t *timestamp)
{
    nanocbor_value_t dec;
    nanocbor_value_t arr;
    uint32_t tag;

    nanocbor_decoder_init(&dec, buf, len);
    nanocbor_get_tag(&dec, &tag);
    /* workaround nanocbor bug see https://github.com/bergzand/NanoCBOR/pull/55 */
    if ((uint16_t)tag != UWB_EPOCH_CBOR_TAG) {
        return -1;
    }
    nanocbor_enter_array(&dec, &arr);
    nanocbor_get_uint32(&arr, timestamp);
    size_t blen;
    const uint8_t *pet;
    nanocbor_get_bstr(&arr, &pet, &blen);
    memcpy(contact->pet.et, pet, blen);
    nanocbor_get_bstr(&arr, &pet, &blen);
    memcpy(contact->pet.rt, pet, blen);
    nanocbor_get_uint16(&arr, &contact->exposure_s);
    nanocbor_get_uint16(&arr, &contact->req_count);
    nanocbor_get_uint8(&arr, &contact->avg_d_cm);
    nanocbor_leave_container(&dec, &arr);
    return 0;
}

void uwb_epoch_data_memory_manager_init(
    uwb_epoch_data_memory_manager_t *manager)
{
    memset(manager, '\0', sizeof(uwb_epoch_data_memory_manager_t));
    memarray_init(&manager->mem, manager->buf, sizeof(uwb_epoch_data_t),
                  CONFIG_UWB_ED_BUF_SIZE);
}

void uwb_epoch_data_memory_manager_free(
    uwb_epoch_data_memory_manager_t *manager,
    uwb_epoch_data_t *uwb_epoch_data)
{
    memarray_free(&manager->mem, uwb_epoch_data);
}

uwb_epoch_data_t *uwb_epoch_data_memory_manager_calloc(
    uwb_epoch_data_memory_manager_t *manager)
{
    return memarray_calloc(&manager->mem);
}
