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
 * @brief       Common BLE Encounter Data code
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */
#include "math.h"
#include "ed.h"
#include "ed_shared.h"
#include "fmt.h"
#include "test_utils/result_output.h"
#include "json_encoder.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

uint16_t ed_ble_rssi_to_cm(float rssi)
{
    #define ALPHA (0.004522930041388781)
    #define BETA (0.8545323033219432)
    return (uint16_t)(ALPHA * pow(BETA, rssi));
}

void ed_ble_set_obf_value(ed_t *ed, ebid_t *ebid)
{
    bool local_gt_remote = false;

    /* compare local ebid and the remote one to see which one
       is greater */
    for (uint8_t i = 0; i < EBID_SIZE; i++) {
        if (ebid->parts.ebid.u8[i] > ed->ebid.parts.ebid.u8[i]) {
            local_gt_remote = true;
            break;
        }
        else if (ebid->parts.ebid.u8[i] < ed->ebid.parts.ebid.u8[i]) {
            break;
        }
    }
    /* calculate obf depending on which ebid is greater */
    if (local_gt_remote) {
        ed->obf = (ebid->parts.ebid.u8[0] << 8) | ebid->parts.ebid.u8[1];
    }
    else {
        ed->obf = (ed->ebid.parts.ebid.u8[0] << 8) | ed->ebid.parts.ebid.u8[1];
    }
    ed->obf %= CONFIG_ED_BLE_OBFUSCATE_MAX;
    LOG_DEBUG("[ed] ble_win: obf value %04" PRIx16 "\n", ed->obf);
}

ed_t *ed_list_process_scan_data(ed_list_t *list, const uint32_t cid, uint16_t time,
                                int8_t rssi)
{
    ed_t *ed = ed_list_get_by_cid(list, cid);

    if (!ed) {
        LOG_WARNING("[ed] ble_win: could not find by cid\n");
    }
    else {
        /* only add data once ebid was reconstructed */
        if (ed->ebid.status.status == EBID_HAS_ALL) {
#if IS_USED(MODULE_ED_BLE_WIN)
            ed_ble_win_process_data(ed, time, rssi);
#endif
#if IS_USED(MODULE_ED_BLE)
            ed_ble_process_data(ed, time, rssi);
#endif
        }
    }
    return ed;
}

void ed_serialize_ble_printf(ed_ble_data_t *data, const char *bn)
{
    turo_t ctx;
    char bn_buff[32 + sizeof(":ble:") + 2 * sizeof(uint32_t)];

    /* "pepper_tag:cid_string" */
    if (strlen(bn) > 32) {
        return;
    }

    turo_init(&ctx);
    turo_array_open(&ctx);
    turo_dict_open(&ctx);
    turo_dict_key(&ctx, "bn");
    if (bn) {
        sprintf(bn_buff, "%s:ble:%" PRIx32 "", bn, data->cid);
    }
    else {
        sprintf(bn_buff, "ble:%" PRIx32 "", data->cid);
    }
    turo_string(&ctx, bn_buff);
    turo_dict_key(&ctx, "bt");
    turo_u32(&ctx, data->time);
    turo_dict_key(&ctx, "n");
    turo_string(&ctx, "rssi");
    turo_dict_key(&ctx, "v");
    turo_s32(&ctx, (int32_t)data->rssi);
    turo_dict_key(&ctx, "u");
    turo_string(&ctx, "dBm");
    turo_dict_close(&ctx);
    turo_array_close(&ctx);
    print_str("\n");
}

size_t ed_serialize_ble_json(ed_ble_data_t *data, const char *bn, uint8_t *buf, size_t len)
{
    (void)len;
    json_encoder_t ctx;

    json_encoder_init(&ctx, (char *)buf, len);
    char bn_buff[32 + sizeof(":ble:") + 2 * sizeof(uint32_t)];

    /* "pepper_tag:cid_string" */
    if (strlen(bn) > 32) {
        return 0;
    }

    json_array_open(&ctx);
    json_dict_open(&ctx);
    json_dict_key(&ctx, "bn");
    if (bn) {
        sprintf(bn_buff, "%s:ble:%" PRIx32 "", bn, data->cid);
    }
    else {
        sprintf(bn_buff, "ble:%" PRIx32 "", data->cid);
    }
    json_string(&ctx, bn_buff);
    json_dict_key(&ctx, "bt");
    json_u32(&ctx, data->time);
    json_dict_key(&ctx, "n");
    json_string(&ctx, "rssi");
    json_dict_key(&ctx, "v");
    json_s32(&ctx, (int32_t)data->rssi);
    json_dict_key(&ctx, "u");
    json_string(&ctx, "dBm");
    json_dict_close(&ctx);
    json_array_close(&ctx);
    return json_encoder_end(&ctx);
}

size_t ed_serialize_ble_csv(ed_ble_data_t *ed, const char *bn, char*buf, size_t len)
{
    (void)len;
    int size = 0;

    if (bn) {
        size += fmt_str(buf + size, bn);
    }
    size += fmt_char(buf + size, ',');
    size += fmt_str(buf + size, "ble");
    size += fmt_char(buf + size, ',');
    size += fmt_bytes_hex(buf + size, (uint8_t*)&ed->cid, sizeof(uint32_t));
    size += fmt_char(buf + size, ',');
    size += fmt_u32_dec(buf + size, ed->time);
    size += fmt_char(buf + size, ',');
    size += fmt_float(buf + size, ed->rssi, 2);
    size += fmt_char(buf + size, ',');
    return size;
}

void ed_serialize_ble_printf_csv(ed_ble_data_t *ed, const char *bn)
{
    if (bn) {
        print_str(bn);
    }
    print_str(",");
    print_str("ble");
    print_str(",");
    print_bytes_hex((uint8_t *)&ed->cid, sizeof(uint32_t));
    print_str(",");
    print_u32_dec(ed->time);
    print_str(",");
    print_float(ed->rssi, 2);
    print_str(",");
    print_str("\n");
}
