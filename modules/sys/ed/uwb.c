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
#include "ed.h"
#include "ed_shared.h"
#include "fmt.h"
#include "test_utils/result_output.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

void ed_uwb_process_data(ed_t *ed, uint16_t time, uint16_t d_cm, uint16_t los)
{
    ed->uwb.seen_last_s = time;
    ed->uwb.req_count++;
    ed->uwb.cumulative_d_cm += d_cm;
#if IS_USED(MODULE_ED_UWB_LOS)
    ed->uwb.cumulative_los += los;
#else
    (void)los;
#endif
}

ed_t *ed_list_process_rng_data(ed_list_t *list, const uint16_t addr, uint16_t time,
                               uint16_t d_cm, uint16_t los)
{
    ed_t *ed = ed_list_get_by_short_addr(list, addr);

    if (!ed) {
        LOG_WARNING("[ed]: could not find by addr\n");
    }
    else {
        ed_uwb_process_data(ed, time, d_cm, los);
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
void ed_serialize_uwb_json(uint16_t d_cm, uint16_t los, uint32_t cid, uint32_t time,
                           const char *base_name)
{
    turo_t ctx;
    char cid_buff[2 * sizeof(uint32_t)];

    turo_init(&ctx);
    turo_dict_open(&ctx);
    if (base_name) {
        turo_dict_key(&ctx, "bn");
        turo_string(&ctx, base_name);
    }
    turo_dict_key(&ctx, "t");
    turo_u32(&ctx, time);
    turo_dict_key(&ctx, "n");
    sprintf(cid_buff, "%"PRIx32"", cid);
    turo_string(&ctx, cid_buff);
    turo_dict_key(&ctx, "v");
    turo_u32(&ctx, (uint32_t)d_cm);
    turo_dict_key(&ctx, "u");
    turo_string(&ctx, "cm");
    turo_dict_close(&ctx);
#if IS_USED(MODULE_ED_UWB_LOS)
    turo_dict_open(&ctx);
    if (base_name) {
        turo_dict_key(&ctx, "bn");
        turo_string(&ctx, base_name);
    }
    turo_dict_key(&ctx, "t");
    turo_u32(&ctx, time);
    turo_dict_key(&ctx, "n");
    sprintf(cid_buff, "%"PRIx32"", cid);
    turo_string(&ctx, cid_buff);
    turo_dict_key(&ctx, "v");
    turo_u32(&ctx, (uint32_t)los);
    turo_dict_key(&ctx, "u");
    turo_string(&ctx, "%");
    turo_dict_close(&ctx);
#else
    (void)los;
#endif
    print_str("\n");
}
