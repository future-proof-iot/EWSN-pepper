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
 * @brief       Windowed BLE specific Encounter Data
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */
#include "ed.h"
#include "ed_shared.h"
#include "rdl_window.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_DEBUG
#endif
#include "log.h"

void ed_ble_win_process_data(ed_t *ed, uint16_t time, int8_t rssi)
{
    float rssi_f = (float)rssi;
    ed->ble_win.seen_last_s = time;
    if (IS_ACTIVE(CONFIG_ED_BLE_OBFUSCATE_RSSI)) {
        /* obfuscate rssi value */
        rssi_f = rssi_f - ed->obf - CONFIG_ED_BLE_RX_COMPENSATION_GAIN;
    }
    rdl_windows_update(&ed->ble_win.wins, rssi_f, time);
}

bool ed_ble_win_finish(ed_t *ed)
{
    /* if exposure time was enough then the ebid must have been reconstructed */
    uint16_t exposure = ed->ble_win.seen_last_s - ed->ble_win.seen_first_s;
    /* convertion could be avoided if exposure is not enough, it is done here
       to be able to compare with UWB results even if invalid */
    rdl_windows_finalize(&ed->ble_win.wins);
    if (exposure >= MIN_EXPOSURE_TIME_S) {
        ed->ble_win.valid = true;
        return true;
    }
    LOG_DEBUG("[ed] ble_win: not enough exposure: %" PRIu16 "s\n", exposure);
    return false;
}
