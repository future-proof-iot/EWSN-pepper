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
 * @brief       BLE specific Encounter Data
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <math.h>
#include "ed.h"
#include "ed_shared.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_ERROR
#endif
#include "log.h"

void ed_ble_process_data(ed_t *ed, uint16_t time, int8_t rssi)
{
    if (rssi >= RSSI_CLIPPING_THRESH) {
        rssi = RSSI_CLIPPING_THRESH;
    }

    float rssi_f = (float)rssi;

    ed->ble.seen_last_s = time;
    if (IS_ACTIVE(CONFIG_ED_BLE_OBFUSCATE_RSSI)) {
        /* obfuscate rssi value */
        rssi_f = rssi_f - ed->obf - CONFIG_ED_BLE_RX_COMPENSATION_GAIN;
    }
    float value = pow(10.0, rssi_f / 10.0);

    ed->ble.cumulative_rssi += value;
    ed->ble.scan_count++;
}

bool ed_ble_finish(ed_t *ed, uint32_t min_exposure_s)
{
    /* if exposure time was enough then the ebid must have been reconstructed */
    uint16_t exposure = ed->ble.seen_last_s - ed->ble.seen_first_s;

    /* convertion could be avoided if exposure is not enough, it is done here
       to be able to compare with UWB results even if invalid */
    /* normalized average */
    if (ed->ble.scan_count > 0) {
        float n_avg = ed->ble.cumulative_rssi / ed->ble.scan_count;
        /* set the cummulative_rssi to the rssi average */
        ed->ble.cumulative_rssi = 10 * log10f(n_avg);
        ed->ble.cumulative_d_cm = ed_ble_rssi_to_cm(ed->ble.cumulative_rssi);
        if (ed->ble.cumulative_d_cm <= MAX_DISTANCE_CM) {
            if (exposure >= min_exposure_s) {
                if (ed->ble.scan_count >= MIN_REQUEST_COUNT) {
                    ed->ble.valid = true;
                    return true;
                }
                else {
                    LOG_DEBUG("[ed] ble: not enough scans: %" PRIu16 "\n",
                              ed->ble.scan_count);
                }
            }
            else {
                LOG_DEBUG("[ed] ble: not enough exposure: %" PRIu16 "s\n",
                          exposure);
            }
        }
        LOG_DEBUG("[ed] ble: not close enough: %" PRIu32 "cm\n",
                  ed->ble.cumulative_d_cm);
    }
    return false;
}
