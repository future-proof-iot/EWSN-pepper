
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
        if (epoch_valid_contact(&epoch->contacts[i])) {
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
#if IS_USED(MODULE_ED_UWB_RSSI)
            json_dict_key(&ctx, "avg_rssi");
            json_float(&ctx, epoch->contacts[i].uwb.avg_rssi);
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
        if (epoch_valid_contact(&epoch->contacts[i])) {
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
#if IS_USED(MODULE_ED_UWB_RSSI)
            turo_dict_key(&ctx, "avg_rssi");
            turo_float(&ctx, epoch->contacts[i].uwb.avg_rssi);
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
        nanocbor_fmt_array(&enc,
                           2 + IS_USED(MODULE_ED_UWB) + IS_USED(MODULE_ED_BLE) * IS_ACTIVE(
                               CONFIG_CONTACT_DATA_SERIALIZE_CBOR_BLE));
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
        if (IS_ACTIVE(CONFIG_CONTACT_DATA_SERIALIZE_CBOR_BLE)) {
            nanocbor_fmt_tag(&enc, ED_BLE_CBOR_TAG);
            nanocbor_fmt_array(&enc, 4);
            nanocbor_fmt_uint(&enc, epoch->contacts[i].ble.exposure_s);
            nanocbor_fmt_uint(&enc, epoch->contacts[i].ble.scan_count);
            nanocbor_fmt_uint(&enc, epoch->contacts[i].ble.avg_d_cm);
            nanocbor_fmt_float(&enc, epoch->contacts[i].ble.avg_rssi);
        }
#endif
    }
    return nanocbor_encoded_len(&enc);
}

int contact_data_load_all_cbor(uint8_t *buf, size_t len, epoch_data_t *epoch)
{
    nanocbor_value_t dec;
    nanocbor_value_t arr1;
    nanocbor_value_t arr2;
    nanocbor_value_t arr3;
    nanocbor_value_t arr4;

    (void)arr4;
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
        while (!nanocbor_at_end(&arr3)) {
            nanocbor_get_tag(&arr3, &tag);
#if IS_USED(MODULE_ED_UWB)
            if (tag == ED_UWB_CBOR_TAG) {
                nanocbor_enter_array(&arr3, &arr4);
                nanocbor_get_uint16(&arr4, &epoch->contacts[i].uwb.exposure_s);
                nanocbor_get_uint16(&arr4, &epoch->contacts[i].uwb.req_count);
                nanocbor_get_uint16(&arr4, &epoch->contacts[i].uwb.avg_d_cm);
                nanocbor_leave_container(&arr3, &arr4);
            }
#endif
#if IS_USED(MODULE_ED_BLE)
            if (IS_ACTIVE(CONFIG_CONTACT_DATA_SERIALIZE_CBOR_BLE)) {
                if (tag == ED_BLE_CBOR_TAG) {
                    nanocbor_enter_array(&arr3, &arr4);

                    nanocbor_get_uint16(&arr4, &epoch->contacts[i].ble.exposure_s);
                    nanocbor_get_uint16(&arr4, &epoch->contacts[i].ble.scan_count);
                    nanocbor_get_uint16(&arr4, &epoch->contacts[i].ble.avg_d_cm);
                    /* no get float in nanocbor */
                    // nanocbor_get_float(&arr4, &epoch->contacts[i].ble.avg_rssi);
                    nanocbor_skip_simple(&arr4);
                    nanocbor_leave_container(&arr3, &arr4);
                }
            }
#endif
        }
        nanocbor_leave_container(&arr2, &arr3);
    }
    nanocbor_leave_container(&arr1, &arr2);
    nanocbor_leave_container(&dec, &arr1);
    return 0;
}

size_t contact_data_serialize_cbor(contact_data_t *contact, uint32_t timestamp,
                                   uint8_t *buf, size_t len)
{
    nanocbor_encoder_t enc;

    nanocbor_encoder_init(&enc, buf, len);
    nanocbor_fmt_tag(&enc, EPOCH_CBOR_TAG);
    nanocbor_fmt_array(&enc,
                       3 + IS_USED(MODULE_ED_UWB) + IS_USED(MODULE_ED_BLE) * IS_ACTIVE(
                           CONFIG_CONTACT_DATA_SERIALIZE_CBOR_BLE));
    nanocbor_fmt_uint(&enc, timestamp);
    nanocbor_put_bstr(&enc, contact->pet.et, PET_SIZE);
    nanocbor_put_bstr(&enc, contact->pet.rt, PET_SIZE);
#if IS_USED(MODULE_ED_UWB)
    nanocbor_fmt_tag(&enc, ED_UWB_CBOR_TAG);
    nanocbor_fmt_array(&enc, 3);
    nanocbor_fmt_uint(&enc, contact->uwb.exposure_s);
    nanocbor_fmt_uint(&enc, contact->uwb.req_count);
    nanocbor_fmt_uint(&enc, contact->uwb.avg_d_cm);
#endif
#if IS_USED(MODULE_ED_BLE)
    if (IS_ACTIVE(CONFIG_CONTACT_DATA_SERIALIZE_CBOR_BLE)) {
        nanocbor_fmt_tag(&enc, ED_BLE_CBOR_TAG);
        nanocbor_fmt_array(&enc, 4);
        nanocbor_fmt_uint(&enc, contact->ble.exposure_s);
        nanocbor_fmt_uint(&enc, contact->ble.scan_count);
        nanocbor_fmt_uint(&enc, contact->ble.avg_d_cm);
        nanocbor_fmt_float(&enc, contact->ble.avg_rssi);
    }
#endif
    return nanocbor_encoded_len(&enc);
}

int contact_data_load_cbor(uint8_t *buf, size_t len,
                           contact_data_t *contact, uint32_t *timestamp)
{
    nanocbor_value_t dec;
    nanocbor_value_t arr1;
    nanocbor_value_t arr2;

    (void)arr2;
    uint32_t tag;
    size_t blen;
    const uint8_t *pet;

    nanocbor_decoder_init(&dec, buf, len);
    nanocbor_get_tag(&dec, &tag);
    if (tag != EPOCH_CBOR_TAG) {
        return -1;
    }

    nanocbor_enter_array(&dec, &arr1);

    nanocbor_get_uint32(&arr1, timestamp);
    nanocbor_get_bstr(&arr1, &pet, &blen);
    memcpy(contact->pet.et, pet, blen);
    nanocbor_get_bstr(&arr1, &pet, &blen);
    memcpy(contact->pet.rt, pet, blen);

    while (!nanocbor_at_end(&arr1)) {
        nanocbor_get_tag(&arr1, &tag);
#if IS_USED(MODULE_ED_UWB)
        if (tag == ED_UWB_CBOR_TAG) {
            nanocbor_enter_array(&arr1, &arr2);
            nanocbor_get_uint16(&arr2, &contact->uwb.exposure_s);
            nanocbor_get_uint16(&arr2, &contact->uwb.req_count);
            nanocbor_get_uint16(&arr2, &contact->uwb.avg_d_cm);
            nanocbor_leave_container(&arr1, &arr2);
        }
#endif
#if IS_USED(MODULE_ED_BLE)
        if (IS_ACTIVE(CONFIG_CONTACT_DATA_SERIALIZE_CBOR_BLE)) {
            if (tag == ED_BLE_CBOR_TAG) {
                nanocbor_enter_array(&arr1, &arr2);
                nanocbor_get_uint16(&arr2, &contact->ble.exposure_s);
                nanocbor_get_uint16(&arr2, &contact->ble.scan_count);
                nanocbor_get_uint16(&arr2, &contact->ble.avg_d_cm);
                /* no get_float in nanocbor */
                // nanocbor_get_float(&arr3, &epoch->contacts[i].ble.avg_rssi);
                nanocbor_skip_simple(&arr2);
                nanocbor_leave_container(&arr1, &arr2);
            }
        }
#endif
    }
    nanocbor_leave_container(&dec, &arr1);
    return 0;
}

// static char *epoch_json_label(epoch_label_t label)
// {
//     switch (label) {
//     case EPOCH_LABEL_EPOCH:
//         return EPOCH_LABEL_JSON_EPOCH;
//     case EPOCH_LABEL_PETS:
//         return EPOCH_LABEL_JSON_PETS;
//     case EPOCH_LABEL_ET:
//         return EPOCH_LABEL_JSON_ET;
//     case EPOCH_LABEL_RT:
//         return EPOCH_LABEL_JSON_RT;
//     case EPOCH_LABEL_UWB:
//         return EPOCH_LABEL_JSON_UWB;
//     case EPOCH_LABEL_BLE:
//         return EPOCH_LABEL_JSON_BLE;
//     case EPOCH_LABEL_COUNT:
//         return EPOCH_LABEL_JSON_COUNT;
//     case EPOCH_LABEL_DISTANCE:
//         return EPOCH_LABEL_JSON_DISTANCE;
//     case EPOCH_LABEL_RSSI:
//         return EPOCH_LABEL_JSON_RSSI;
//     case EPOCH_LABEL_EXPOSURE:
//         return EPOCH_LABEL_JSON_EXPOSURE;
//     default:
//         return "";
//     }
// }
