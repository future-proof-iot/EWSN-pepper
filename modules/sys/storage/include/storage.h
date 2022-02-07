/*
 * Copyright (C) 2018 Koen Zandberg <koen@bergzand.net>
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_storage Storage
 * @ingroup     sys
 * @brief       Storage Abstraction
 *
 * @{
 *
 * @file
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include <stdlib.h>

#include "board.h"
#include "mtd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STORAGE_FS_MOUNT_POINT              "/sys"

#ifndef CONFIG_STORAGE_FS_FORCE_FORMAT
#define CONFIG_STORAGE_FS_FORCE_FORMAT      0
#endif

int storage_init(void);
int storage_deinit(void);
int storage_log(const char* path, uint8_t *buffer, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_H */
/** @} */
