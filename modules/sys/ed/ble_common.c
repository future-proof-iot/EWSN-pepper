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
#include "ed.h"
#include "ed_shared.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

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
