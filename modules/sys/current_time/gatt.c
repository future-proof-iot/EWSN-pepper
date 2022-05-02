/*
 * Copyright (C) 2022 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_current_time_gatt
 * @{
 *
 * @file
 * @brief       Current Time Gatt service
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include "current_time.h"

#include "net/bluetil/ad.h"
#include "net/bluetil/addr.h"
#include "host/ble_hs.h"
#include "nimble/hci_common.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

