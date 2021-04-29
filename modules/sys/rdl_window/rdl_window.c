/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_rdl_window
 * @{
 *
 * @file
 * @brief       Request Data List Windows implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <string.h>
#include <math.h>
#include "timex.h"

#include "rdl_window.h"

#define ENABLE_DEBUG    0
#include "debug.h"


static const uint32_t _windows[WINDOWS_PER_EPOCH][2] = {
    {  0 * WINDOW_STEP_S,  0 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
    {  1 * WINDOW_STEP_S,  1 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
    {  2 * WINDOW_STEP_S,  2 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
    {  3 * WINDOW_STEP_S,  3 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
    {  4 * WINDOW_STEP_S,  4 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
    {  5 * WINDOW_STEP_S,  5 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
    {  6 * WINDOW_STEP_S,  6 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
    {  7 * WINDOW_STEP_S,  7 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
    {  8 * WINDOW_STEP_S,  8 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
    {  9 * WINDOW_STEP_S,  9 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
    { 10 * WINDOW_STEP_S, 10 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
    { 11 * WINDOW_STEP_S, 11 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
    { 12 * WINDOW_STEP_S, 12 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
    { 13 * WINDOW_STEP_S, 13 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
    { 14 * WINDOW_STEP_S, 14 * WINDOW_STEP_S + WINDOW_LENGTH_S - 1 },
};


void rdl_windows_finalize(rdl_windows_t *wins)
{
    if (IS_ACTIVE(ENABLE_DEBUG)) {
        for (uint8_t i = 0; i < WINDOWS_PER_EPOCH; i++) {
            DEBUG("[rdl_windows]: window[%d] avg: %0.10lf\n", i,
                  wins->wins[i].avg)
        }
    }

    for (uint8_t i = 0; i < WINDOWS_PER_EPOCH; i++) {
        /* if all values are 0 or none do not convert */
        if (wins->wins[i].samples && wins->wins[i].avg) {
            float n_avg = wins->wins[i].avg / wins->wins[i].samples;
            if (IS_ACTIVE(ENABLE_DEBUG)) {
                DEBUG("[rdl_windows]: window[%d] normalized avg: %0.10lf\n", i,
                      n_avg)
            }
            wins->wins[i].avg = 10 * log10f(n_avg);
        }
    }

    if (IS_ACTIVE(ENABLE_DEBUG)) {
        for (uint8_t i = 0; i < WINDOWS_PER_EPOCH; i++) {
            DEBUG(
                "[rdl_windows]: window[%d] avg after log convertion: %0.10lf\n",
                i, wins->wins[i].avg)
        }
    }
}

bool _in_window(int16_t time, int16_t start, int16_t end)
{
    if (time >= start && time <= end) {
        return true;
    }
    return false;
}

void rdl_windows_update(rdl_windows_t *wins, float rssi, int16_t time)
{
    if (rssi >= RSSI_CLIPPING_THRESH) {
        rssi = RSSI_CLIPPING_THRESH;
    }

    for (uint8_t i = 0; i < WINDOWS_PER_EPOCH; i++) {
        if (_in_window(time, _windows[i][0], _windows[i][1])) {
            float value = pow(10.0, rssi / 10.0);
            DEBUG("[rdl_windows]: data %0.10lf added to window %d\n", value, i);
            wins->wins[i].avg += value;
            wins->wins[i].samples++;
            /* once matched the value can at most overlap on the next window
               or not at all if in the first or last window */
            if (i + 1 < WINDOWS_PER_EPOCH - 1) {
                if (_in_window(time, _windows[i + 1][0], _windows[i + 1][1])) {
                    DEBUG("[rdl_windows]: data %0.10lf added to window %d\n",
                          value, i + 1);
                    wins->wins[i + 1].avg += value;
                    wins->wins[i + 1].samples++;
                }
            }
            break;
        }
    }
}
