/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_crypto_manager
 * @{
 *
 * @file
 * @brief       Crypto Manager Common implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <assert.h>

#include "crypto_manager.h"
#include "kernel_defines.h"

#define ENABLE_DEBUG    0
#include "debug.h"

int8_t array_a_greater_than_b(uint8_t *a, uint8_t *b)
{
    for (uint8_t i = 0; i < C25519_KEY_SIZE; i++) {
        if (a[i] > b[i]) {
            return 1;
            break;
        }
        else if (a[i] < b[i]) {
            return 0;
            break;
        }
    }
    return -1;
}
