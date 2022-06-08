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
 * @author      Anonymous
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

/**
 * @brief   Default VFS data storage path
 */
#ifdef MODULE_VFS_DEFAULT
#include "vfs_default.h"
#define VFS_STORAGE_DATA              VFS_DEFAULT_DATA
#else
#define VFS_STORAGE_DATA              "/sda"
#endif

/**
 * @brief   Initialize storage, mount and create directory hierarchy
 *
 * @return 0 on success, <0 otherwise
 */
int storage_init(void);

/**
 * @brief   De-initialize storage (unmount)
 *
 * @return 0 on success, <0 otherwise
 */
int storage_deinit(void);

/**
 * @brief   Log buffer to storage
 *
 * @param path      filesystem path to log the buffer data
 * @param buffer    buffer holding data to log
 * @param len       size of data to log
 *
 * @return 0 on success, <0 otherwise
 */
int storage_log(const char* path, uint8_t *buffer, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_H */
/** @} */
