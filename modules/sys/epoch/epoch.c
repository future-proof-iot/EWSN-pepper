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
#include "fmt.h"
#include "test_utils/result_output.h"
#include "nanocbor/nanocbor.h"
#include "json_encoder.h"

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

static bool _epoch_valid_contact(contact_data_t *data)
{
    bool valid = false;

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
        if (_epoch_valid_contact(&epoch->contacts[i])) {
            contacts++;
        }
    }
    return contacts;
}

static void turo_hexarray(turo_t *ctx, uint8_t *vals, size_t size)
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

size_t contact_data_serialize_all_json(epoch_data_t *epoch, uint8_t *buf,
                                       size_t len, const char *prefix)
{
    json_encoder_t ctx;

    json_encoder_init(&ctx, (char *)buf, len);
    json_dict_open(&ctx);
    if (prefix) {
        json_dict_key(&ctx, "tag");
        json_string(&ctx, prefix);
    }
    json_dict_key(&ctx, "epoch");
    json_u32(&ctx, epoch->timestamp);
    json_dict_key(&ctx, "pets");
    json_array_open(&ctx);
    for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
        if (_epoch_valid_contact(&epoch->contacts[i])) {
            json_dict_open(&ctx);
            json_dict_key(&ctx, "pet");
            json_dict_open(&ctx);
            json_dict_key(&ctx, "etl");
            json_hexarray(&ctx, epoch->contacts[i].pet.et, PET_SIZE);
            json_dict_key(&ctx, "rtl");
            json_hexarray(&ctx, epoch->contacts[i].pet.rt, PET_SIZE);
#if IS_USED(MODULE_ED_UWB)
            json_dict_key(&ctx, "uwb");
            json_dict_open(&ctx);
            json_dict_key(&ctx, "exposure");
            json_u32(&ctx, epoch->contacts[i].uwb.exposure_s);
#if IS_USED(MODULE_ED_UWB_STATS)
            json_dict_key(&ctx, "lst_scheduled");
            json_u32(&ctx, epoch->contacts[i].uwb.stats.lst.scheduled);
            json_dict_key(&ctx, "lst_aborted");
            json_u32(&ctx, epoch->contacts[i].uwb.stats.lst.aborted);
            json_dict_key(&ctx, "lst_timeout");
            json_u32(&ctx, epoch->contacts[i].uwb.stats.lst.timeout);
            json_dict_key(&ctx, "req_scheduled");
            json_u32(&ctx, epoch->contacts[i].uwb.stats.req.scheduled);
            json_dict_key(&ctx, "req_aborted");
            json_u32(&ctx, epoch->contacts[i].uwb.stats.req.aborted);
            json_dict_key(&ctx, "req_timeout");
            json_u32(&ctx, epoch->contacts[i].uwb.stats.req.timeout);
#endif
            json_dict_key(&ctx, "req_count");
            json_u32(&ctx, epoch->contacts[i].uwb.req_count);
            json_dict_key(&ctx, "avg_d_cm");
            json_u32(&ctx, epoch->contacts[i].uwb.avg_d_cm);
#if IS_USED(MODULE_ED_UWB_LOS)
            json_dict_key(&ctx, "avg_los");
            json_u32(&ctx, epoch->contacts[i].uwb.avg_los);
#endif
            json_dict_close(&ctx);
#endif
#if IS_USED(MODULE_ED_BLE)
            json_dict_key(&ctx, "ble");
            json_dict_open(&ctx);
            json_dict_key(&ctx, "exposure");
            json_u32(&ctx, epoch->contacts[i].ble.exposure_s);
            json_dict_key(&ctx, "scan_count");
            json_u32(&ctx, epoch->contacts[i].ble.scan_count);
            json_dict_key(&ctx, "avg_rssi");
            json_float(&ctx, epoch->contacts[i].ble.avg_rssi);
            json_dict_key(&ctx, "avg_d_cm");
            json_u32(&ctx, epoch->contacts[i].ble.avg_d_cm);

            json_dict_close(&ctx);
#endif
#if IS_USED(MODULE_ED_BLE_WIN)
            json_dict_key(&ctx, "ble_win");
            json_dict_open(&ctx);
            json_dict_key(&ctx, "exposure");
            json_u32(&ctx, epoch->contacts[i].ble_win.exposure_s);
            json_dict_key(&ctx, "wins");
            json_array_open(&ctx);
            for (uint8_t j = 0; j < WINDOWS_PER_EPOCH; j++) {
                json_dict_open(&ctx);
                json_dict_key(&ctx, "samples");
                json_u32(&ctx, epoch->contacts[i].ble_win.wins[j].samples);
                json_dict_key(&ctx, "rssi");
                json_float(&ctx, epoch->contacts[i].ble_win.wins[j].avg);
                json_dict_close(&ctx);
            }
            json_array_close(&ctx);
            json_dict_close(&ctx);
#endif
            json_dict_close(&ctx);
            json_dict_close(&ctx);
        }
    }
    json_array_close(&ctx);
    json_dict_close(&ctx);
    return json_encoder_end(&ctx);
}


void contact_data_serialize_all_printf(epoch_data_t *epoch, const char *prefix)
{
    turo_t ctx;

    /* disable irq to avoid scrambled logs */
    unsigned int state = irq_disable();

    turo_init(&ctx);
    turo_dict_open(&ctx);
    if (prefix) {
        turo_dict_key(&ctx, "tag");
        turo_string(&ctx, prefix);
    }
    turo_dict_key(&ctx, "epoch");
    turo_u32(&ctx, epoch->timestamp);
    turo_dict_key(&ctx, "pets");
    turo_array_open(&ctx);
    for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
        if (_epoch_valid_contact(&epoch->contacts[i])) {
            turo_dict_open(&ctx);
            turo_dict_key(&ctx, "pet");
            turo_dict_open(&ctx);
            turo_dict_key(&ctx, "etl");
            turo_hexarray(&ctx, epoch->contacts[i].pet.et, PET_SIZE);
            turo_dict_key(&ctx, "rtl");
            turo_hexarray(&ctx, epoch->contacts[i].pet.rt, PET_SIZE);
#if IS_USED(MODULE_ED_UWB)
            turo_dict_key(&ctx, "uwb");
            turo_dict_open(&ctx);
            turo_dict_key(&ctx, "exposure");
            turo_u32(&ctx, epoch->contacts[i].uwb.exposure_s);
#if IS_USED(MODULE_ED_UWB_STATS)
            turo_dict_key(&ctx, "lst_scheduled");
            turo_u32(&ctx, epoch->contacts[i].uwb.stats.lst.scheduled);
            turo_dict_key(&ctx, "lst_aborted");
            turo_u32(&ctx, epoch->contacts[i].uwb.stats.lst.aborted);
            turo_dict_key(&ctx, "lst_timeout");
            turo_u32(&ctx, epoch->contacts[i].uwb.stats.lst.timeout);
            turo_dict_key(&ctx, "req_scheduled");
            turo_u32(&ctx, epoch->contacts[i].uwb.stats.req.scheduled);
            turo_dict_key(&ctx, "req_aborted");
            turo_u32(&ctx, epoch->contacts[i].uwb.stats.req.aborted);
            turo_dict_key(&ctx, "req_timeout");
            turo_u32(&ctx, epoch->contacts[i].uwb.stats.req.timeout);
#endif
            turo_dict_key(&ctx, "req_count");
            turo_u32(&ctx, epoch->contacts[i].uwb.req_count);
            turo_dict_key(&ctx, "avg_d_cm");
            turo_u32(&ctx, epoch->contacts[i].uwb.avg_d_cm);
#if IS_USED(MODULE_ED_UWB_LOS)
            turo_dict_key(&ctx, "avg_los");
            turo_u32(&ctx, epoch->contacts[i].uwb.avg_los);
#endif
            turo_dict_close(&ctx);
#endif
#if IS_USED(MODULE_ED_BLE)
            turo_dict_key(&ctx, "ble");
            turo_dict_open(&ctx);
            turo_dict_key(&ctx, "exposure");
            turo_u32(&ctx, epoch->contacts[i].ble.exposure_s);
            turo_dict_key(&ctx, "scan_count");
            turo_u32(&ctx, epoch->contacts[i].ble.scan_count);
            turo_dict_key(&ctx, "avg_rssi");
            turo_float(&ctx, epoch->contacts[i].ble.avg_rssi);
            turo_dict_key(&ctx, "avg_d_cm");
            turo_u32(&ctx, epoch->contacts[i].ble.avg_d_cm);
            turo_dict_close(&ctx);
#endif
#if IS_USED(MODULE_ED_BLE_WIN)
            turo_dict_key(&ctx, "ble_win");
            turo_dict_open(&ctx);
            turo_dict_key(&ctx, "exposure");
            turo_u32(&ctx, epoch->contacts[i].ble_win.exposure_s);
            turo_dict_key(&ctx, "wins");
            turo_array_open(&ctx);
            for (uint8_t j = 0; j < WINDOWS_PER_EPOCH; j++) {
                turo_dict_open(&ctx);
                turo_dict_key(&ctx, "samples");
                turo_u32(&ctx, epoch->contacts[i].ble_win.wins[j].samples);
                turo_dict_key(&ctx, "rssi");
                turo_float(&ctx, epoch->contacts[i].ble_win.wins[j].avg);
                turo_dict_close(&ctx);
            }
            turo_array_close(&ctx);
            turo_dict_close(&ctx);
#endif
            turo_dict_close(&ctx);
            turo_dict_close(&ctx);
        }
    }
    turo_array_close(&ctx);
    turo_dict_close(&ctx);
    print_str("\n");
    irq_restore(state);
}

size_t contact_data_serialize_all_cbor(epoch_data_t *epoch, uint8_t *buf,
                                       size_t len)
{
    nanocbor_encoder_t enc;

    nanocbor_encoder_init(&enc, buf, len);
    nanocbor_fmt_tag(&enc, EPOCH_CBOR_TAG);
    nanocbor_fmt_array(&enc, 2);
    nanocbor_fmt_uint(&enc, epoch->timestamp);
    uint8_t contacts = epoch_contacts(epoch);

    nanocbor_fmt_array(&enc, contacts);
    for (uint8_t i = 0; i < contacts; i++) {
        nanocbor_fmt_array(&enc, 2 + 2 * IS_USED(MODULE_ED_UWB) + 2 * IS_ACTIVE(MODULE_ED_BLE_WIN));
        nanocbor_put_bstr(&enc, epoch->contacts[i].pet.et, PET_SIZE);
        nanocbor_put_bstr(&enc, epoch->contacts[i].pet.rt, PET_SIZE);
#if IS_USED(MODULE_ED_UWB)
        nanocbor_fmt_tag(&enc, ED_UWB_CBOR_TAG);
        nanocbor_fmt_array(&enc, 3);
        nanocbor_fmt_uint(&enc, epoch->contacts[i].uwb.exposure_s);
        nanocbor_fmt_uint(&enc, epoch->contacts[i].uwb.req_count);
        nanocbor_fmt_uint(&enc, epoch->contacts[i].uwb.avg_d_cm);
#endif
#if IS_USED(MODULE_ED_BLE)
        nanocbor_fmt_tag(&enc, ED_BLE_CBOR_TAG);
        nanocbor_fmt_array(&enc, 3);
        nanocbor_fmt_uint(&enc, epoch->contacts[i].ble.exposure_s);
        nanocbor_fmt_uint(&enc, epoch->contacts[i].ble.scan_count);
        nanocbor_fmt_uint(&enc, epoch->contacts[i].ble.avg_d_cm);
        nanocbor_fmt_float(&enc, epoch->contacts[i].ble.avg_rssi);
#endif
// #if IS_USED(MODULE_ED_BLE_WIN)
//         nanocbor_fmt_tag(&enc, ED_BLE_WIN_CBOR_TAG);
//         nanocbor_fmt_array(&enc, 2);
//         nanocbor_fmt_uint(&enc, epoch->contacts[i].ble_win.exposure_s);
//         nanocbor_fmt_array(&enc, WINDOWS_PER_EPOCH);
//         for (uint8_t j = 0; j < WINDOWS_PER_EPOCH; j++) {
//             nanocbor_fmt_uint(&enc, epoch->contacts[i].ble_win.wins[j].samples);
//             nanocbor_fmt_uint(&enc, epoch->contacts[i].ble_win.wins[j].avg);
//         }
// #endif
    }
    return nanocbor_encoded_len(&enc);
}

int contact_data_load_all_cbor(uint8_t *buf, size_t len, epoch_data_t *epoch)
{
    nanocbor_value_t dec;
    nanocbor_value_t arr1;
    nanocbor_value_t arr2;
    nanocbor_value_t arr3;
    uint32_t tag;

    nanocbor_decoder_init(&dec, buf, len);
    nanocbor_get_tag(&dec, &tag);
    if (tag != EPOCH_CBOR_TAG) {
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
#if IS_USED(MODULE_ED_UWB)
        nanocbor_get_tag(&dec, &tag);
        if (tag == ED_UWB_CBOR_TAG) {
            nanocbor_get_uint16(&arr3, &epoch->contacts[i].uwb.exposure_s);
            nanocbor_get_uint16(&arr3, &epoch->contacts[i].uwb.req_count);
            nanocbor_get_uint16(&arr3, &epoch->contacts[i].uwb.avg_d_cm);
            nanocbor_leave_container(&arr2, &arr3);
        }
#endif
    }
    nanocbor_leave_container(&arr1, &arr2);
    nanocbor_leave_container(&dec, &arr1);
    return 0;
}

// TODO: allow to serialize one contact at a time, similar to SenML
// size_t epoch_serialize_cbor(epoch_data_t *contact, uint32_t timestamp,
//                                    uint8_t *buf, size_t len);
// {
//     nanocbor_encoder_t enc;

//     nanocbor_encoder_init(&enc, buf, len);
//     nanocbor_fmt_tag(&enc, EPOCH_CBOR_TAG);
//     nanocbor_fmt_array(&enc, 6);
//     nanocbor_fmt_uint(&enc, timestamp);
//     nanocbor_put_bstr(&enc, contact->pet.et, PET_SIZE);
//     nanocbor_put_bstr(&enc, contact->pet.rt, PET_SIZE);
//     nanocbor_fmt_uint(&enc, contact->exposure_s);
//     nanocbor_fmt_uint(&enc, contact->req_count);
//     nanocbor_fmt_uint(&enc, contact->avg_d_cm);
//     return nanocbor_encoded_len(&enc);
// }

// int uwb_contact_load_cbor(uint8_t *buf, size_t len,
//                           uwb_contact_data_t *contact, uint32_t *timestamp)
// {
//     nanocbor_value_t dec;
//     nanocbor_value_t arr;
//     uint32_t tag;

//     nanocbor_decoder_init(&dec, buf, len);
//     nanocbor_get_tag(&dec, &tag);
//     nanocbor_enter_array(&dec, &arr);
//     nanocbor_get_uint32(&arr, timestamp);
//     size_t blen;
//     const uint8_t *pet;

//     nanocbor_get_bstr(&arr, &pet, &blen);
//     memcpy(contact->pet.et, pet, blen);
//     nanocbor_get_bstr(&arr, &pet, &blen);
//     memcpy(contact->pet.rt, pet, blen);
//     nanocbor_get_uint16(&arr, &contact->exposure_s);
//     nanocbor_get_uint16(&arr, &contact->req_count);
//     nanocbor_get_uint8(&arr, &contact->avg_d_cm);
//     nanocbor_leave_container(&dec, &arr);
//     return 0;
// }

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
