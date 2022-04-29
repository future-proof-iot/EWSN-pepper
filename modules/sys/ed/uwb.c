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
 * @brief       UWB specific Encounter Data
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <math.h>
#include "ed.h"
#include "ed_shared.h"
#include "fmt.h"
#include "test_utils/result_output.h"

#include "json_encoder.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

void ed_uwb_process_data(ed_t *ed, uint16_t time, uint16_t d_cm, uint16_t los, float rssi)
{
    ed->uwb.seen_last_s = time;
    ed->uwb.req_count++;
    ed->uwb.cumulative_d_cm += d_cm;
#if IS_USED(MODULE_ED_UWB_LOS)
    ed->uwb.cumulative_los += los;
#else
    (void)los;
#endif
#if IS_USED(MODULE_ED_UWB_RSSI)
    float value = pow(10.0, rssi / 10.0);
    ed->uwb.cumulative_rssi += value;
#else
    (void)rssi;
#endif
}

ed_t *ed_list_process_rng_data(ed_list_t *list, const uint16_t addr, uint16_t time,
                               uint16_t d_cm, uint16_t los, float rssi)
{
    ed_t *ed = ed_list_get_by_short_addr(list, addr);

    if (!ed) {
        LOG_WARNING("[ed]: could not find by addr\n");
    }
    else {
        ed_uwb_process_data(ed, time, d_cm, los, rssi);
    }
    return ed;
}

bool ed_uwb_finish(ed_t *ed, uint32_t min_exposure_s)
{
    uint16_t exposure = ed->uwb.seen_last_s - ed->uwb.seen_first_s;

    if (ed->uwb.req_count > 0) {
        LOG_DEBUG("[ed] uwb: sum %" PRIu32 "cm, count %" PRIu16 "\n",
                  ed->uwb.cumulative_d_cm, ed->uwb.req_count);
        ed->uwb.cumulative_d_cm = ed->uwb.cumulative_d_cm / ed->uwb.req_count;
#if IS_USED(MODULE_ED_UWB_LOS)
        ed->uwb.cumulative_los = ed->uwb.cumulative_los / ed->uwb.req_count;
#endif
#if IS_USED(MODULE_ED_UWB_RSSI)
        float n_avg = ed->uwb.cumulative_rssi / ed->uwb.req_count;
        /* set the cummulative_rssi to the rssi average */
        ed->uwb.cumulative_rssi = 10 * log10f(n_avg);
#endif
        if (ed->uwb.cumulative_d_cm <= MAX_DISTANCE_CM) {
            if (exposure >= min_exposure_s) {
                if (ed->uwb.req_count >= MIN_REQUEST_COUNT) {
                    ed->uwb.valid = true;
                    return true;
                }
                else {
                    LOG_DEBUG("[ed] uwb: not enough requests: %" PRIu16 "\n",
                              ed->uwb.req_count);
                }
            }
            else {
                LOG_DEBUG("[ed] uwb: not enough exposure: %" PRIu16 "s\n",
                          exposure);
            }
        }
        LOG_DEBUG("[ed] uwb: not close enough: %" PRIu32 "cm\n",
                  ed->uwb.cumulative_d_cm);

    }
    return false;
}

/* TODO: style this more in SenML, the name should not be the cid, and the bn either */
void ed_serialize_uwb_printf(ed_uwb_data_t *ed, const char *bn)
{
    turo_t ctx;
    char bn_buff[32 + sizeof(":uwb:") + 2 * sizeof(uint32_t)];

    /* "pepper_tag:cid_string" */
    if (strlen(bn) > 32) {
        return;
    }

    turo_init(&ctx);
    turo_array_open(&ctx);
    turo_dict_open(&ctx);
    turo_dict_key(&ctx, "bn");
    if (bn) {
        sprintf(bn_buff, "%s:uwb:%" PRIx32 "", bn, ed->cid);
    }
    else {
        sprintf(bn_buff, "uwb:%" PRIx32 "", ed->cid);
    }
    turo_string(&ctx, bn_buff);
    turo_dict_key(&ctx, "bt");
    turo_u32(&ctx, ed->time);
    turo_dict_key(&ctx, "n");
    turo_string(&ctx, "d_cm");
    turo_dict_key(&ctx, "v");
    turo_u32(&ctx, (uint32_t)ed->d_cm);
    turo_dict_key(&ctx, "u");
    turo_string(&ctx, "cm");
    turo_dict_close(&ctx);
#if IS_USED(MODULE_ED_UWB_LOS)
    turo_dict_open(&ctx);
    turo_dict_key(&ctx, "n");
    turo_string(&ctx, "los");
    turo_dict_key(&ctx, "v");
    turo_u32(&ctx, (uint32_t)ed->los);
    turo_dict_key(&ctx, "u");
    turo_string(&ctx, "%");
    turo_dict_close(&ctx);
#endif
#if IS_USED(MODULE_ED_UWB_RSSI)
    turo_dict_open(&ctx);
    turo_dict_key(&ctx, "n");
    turo_string(&ctx, "rssi");
    turo_dict_key(&ctx, "v");
    turo_float(&ctx, ed->rssi);
    turo_dict_key(&ctx, "u");
    turo_string(&ctx, "dBm");
    turo_dict_close(&ctx);
#endif
    turo_array_close(&ctx);
    print_str("\n");
}

size_t ed_serialize_uwb_json(ed_uwb_data_t *ed, const char *bn, uint8_t *buf, size_t len)
{
    json_encoder_t ctx;

    json_encoder_init(&ctx, (char *)buf, len);
    char bn_buff[32 + sizeof(":uwb:") + 2 * sizeof(uint32_t)];

    /* "pepper_tag:cid_string" */
    if (strlen(bn) > 32) {
        return 0;
    }
    json_array_open(&ctx);
    json_dict_open(&ctx);
    json_dict_key(&ctx, "bn");
    if (bn) {
        sprintf(bn_buff, "%s:uwb:%" PRIx32 "", bn, ed->cid);
    }
    else {
        sprintf(bn_buff, "uwb:%" PRIx32 "", ed->cid);
    }
    json_string(&ctx, bn_buff);
    json_dict_key(&ctx, "bt");
    json_u32(&ctx, ed->time);
    json_dict_key(&ctx, "n");
    json_string(&ctx, "d_cm");
    json_dict_key(&ctx, "v");
    json_u32(&ctx, (uint32_t)ed->d_cm);
    json_dict_key(&ctx, "u");
    json_string(&ctx, "cm");
    json_dict_close(&ctx);
#if IS_USED(MODULE_ED_UWB_LOS)
    json_dict_open(&ctx);
    json_dict_key(&ctx, "n");
    json_string(&ctx, "los");
    json_dict_key(&ctx, "v");
    json_u32(&ctx, (uint32_t)ed->los);
    json_dict_key(&ctx, "u");
    json_string(&ctx, "%");
    json_dict_close(&ctx);
#endif
#if IS_USED(MODULE_ED_UWB_RSSI)
    json_dict_open(&ctx);
    json_dict_key(&ctx, "n");
    json_string(&ctx, "rssi");
    json_dict_key(&ctx, "v");
    json_float(&ctx, ed->rssi);
    json_dict_key(&ctx, "u");
    json_string(&ctx, "dBm");
    json_dict_close(&ctx);
#endif
    json_array_close(&ctx);
    return json_encoder_end(&ctx);
}

size_t ed_serialize_uwb_csv(ed_uwb_data_t *ed, const char *bn, char *buf, size_t len)
{
    (void)len;
    int size = 0;

    if (bn) {
        size += fmt_str(buf + size, bn);
    }
    size += fmt_char(buf+=size, ',');
    size += fmt_str(buf + size, "uwb");
    size += fmt_char(buf + size, ',');
    size += fmt_bytes_hex(buf + size, (uint8_t*)&ed->cid, sizeof(uint32_t));
    size += fmt_char(buf + size, ',');
    size += fmt_u32_dec(buf + size, ed->time);
    size += fmt_char(buf + size, ',');
    size += fmt_u32_dec(buf + size, ed->d_cm);
    size += fmt_char(buf + size, ',');
#if IS_USED(MODULE_ED_UWB_LOS)
    size += fmt_u32_dec(buf + size, ed->los);
#else
    size += fmt_str(buf + size, "NULL");
#endif
    size += fmt_char(buf + size, ',');
#if IS_USED(MODULE_ED_UWB_RSSI)
    size += fmt_float(buf + size, ed->rssi, 2);
#else
    size += fmt_str(buf + size, "NULL");
#endif
    size += fmt_char(buf + size, ',');
    return size;
}

void ed_serialize_uwb_printf_csv(ed_uwb_data_t *ed, const char *bn)
{
    if (bn) {
        print_str(bn);
    }
    print_str(",");
    print_str("uwb");
    print_str(",");
    print_bytes_hex((uint8_t *)&ed->cid, sizeof(uint32_t));
    print_str(",");
    print_u32_dec(ed->time);
    print_str(",");
    print_u32_dec(ed->d_cm);
    print_str(",");
#if IS_USED(MODULE_ED_UWB_LOS)
    print_u32_dec(ed->los);
#else
    print_str("NULL");
#endif
    print_str(",");
#if IS_USED(MODULE_ED_UWB_RSSI)
    print_float(ed->rssi, 2);
#else
    print_str("NULL");
#endif
    print_str(",");
    print_str("\n");
}
