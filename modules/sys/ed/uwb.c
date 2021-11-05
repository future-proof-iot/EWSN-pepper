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

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

void ed_uwb_process_data(ed_t *ed, uint16_t time, uint16_t d_cm)
{
    ed->uwb.seen_last_s = time;
    ed->uwb.req_count++;
    ed->uwb.cumulative_d_cm += d_cm;
}

ed_t *ed_list_process_rng_data(ed_list_t *list, const uint16_t addr, uint16_t time,
                               uint16_t d_cm)
{
    ed_t *ed = ed_list_get_by_short_addr(list, addr);

    if (!ed) {
        LOG_WARNING("[ed]: could not find by addr\n");
    }
    else {
        ed_uwb_process_data(ed, time, d_cm);
    }
    return ed;
}

bool ed_uwb_finish(ed_t *ed)
{
    uint16_t exposure = ed->uwb.seen_last_s - ed->uwb.seen_first_s;

    if (ed->uwb.req_count > 0) {
        LOG_DEBUG("[ed] uwb: sum %" PRIu32 "cm, count %" PRIu16 "\n",
                  ed->uwb.cumulative_d_cm, ed->uwb.req_count);
        ed->uwb.cumulative_d_cm = ed->uwb.cumulative_d_cm / ed->uwb.req_count;
        if (ed->uwb.cumulative_d_cm <= MAX_DISTANCE_CM) {
            if (exposure >= MIN_EXPOSURE_TIME_S) {
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
    }
    LOG_DEBUG("[ed] uwb: not close enough: %" PRIu32 "cm\n",
              ed->uwb.cumulative_d_cm);
    return false;
}