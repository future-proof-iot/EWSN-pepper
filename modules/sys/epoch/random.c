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
 * @brief       Random Epochs
 *
 * @author      Anonymous
 *
 * @}
 */

#include <string.h>
#include <assert.h>

#include "ztimer.h"
#include "random.h"

#include "epoch.h"
#include "ed.h"

void random_contact(contact_data_t *data)
{
    random_bytes(data->pet.et, PET_SIZE);
    random_bytes(data->pet.rt, PET_SIZE);
#if IS_USED(MODULE_ED_UWB)
    data->uwb.avg_d_cm = random_uint32_range(0, MAX_DISTANCE_CM);
    data->uwb.req_count = random_uint32_range(10, 100);
    data->uwb.exposure_s = random_uint32_range(MIN_EXPOSURE_TIME_S,
                                               CONFIG_EPOCH_DURATION_SEC);
#endif
#if IS_USED(MODULE_ED_BLE)
    data->ble.avg_rssi = -1.0 * (float)random_uint32_range(0, 90);
    data->ble.avg_d_cm = random_uint32_range(0, MAX_DISTANCE_CM);
    data->ble.scan_count = random_uint32_range(10, 100);
    data->ble.exposure_s = random_uint32_range(MIN_EXPOSURE_TIME_S,
                                               CONFIG_EPOCH_DURATION_SEC);
#endif
#if IS_USED(MODULE_ED_BLE_WIN)
    data->ble_win.exposure_s = random_uint32_range(MIN_EXPOSURE_TIME_S,
                                                   CONFIG_EPOCH_DURATION_SEC);
    /* mock windowdata BLE data that will pass contact filter */
    for (uint8_t j = 0; j < WINDOWS_PER_EPOCH; j++) {
        data->ble_win.wins[j].samples = (uint16_t)random_uint32_range(1, 1000);
        data->ble_win.wins[j].avg = -1.0 * (float)random_uint32_range(0, 90);
    }
#endif
}

void random_epoch(epoch_data_t *data)
{
    memset(data, '\0', sizeof(epoch_data_t));
    data->timestamp = (uint16_t)ztimer_now(ZTIMER_EPOCH);
    for (uint8_t i = 0; i < 2; i++) {
        random_contact(&data->contacts[i]);
    }
}
