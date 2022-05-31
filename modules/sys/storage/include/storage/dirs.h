/*
 * Copyright (C) 2018 Koen Zandberg <koen@bergzand.net>
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_storage_dirs Storage Directories
 * @ingroup     sys
 * @brief       Storage Abstraction
 *
 * @{
 *
 * @file
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef STORAGE_DIRS_H
#define STORAGE_DIRS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   forward declaration for storage_dit_t
 */
typedef struct _storage_dir storage_dir_t;

/**
 * @brief   Storage directory tree structure definition
 */
struct _storage_dir {
    const storage_dir_t *subdirs;   /**< subdirectories */
    const char *name;               /**< directory name */
};

/**
 * @brief   Create storage directory from hierarchy
 *
 * @param hierarchy
 * @param prefix
 * @return
 */
int storage_dirs_create_hier(const storage_dir_t *hierarchy, const char *prefix);

/**
 * @brief
 *
 * @return int
 */
int storage_dirs_create_sys_hier(void);

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_DIRS_H */
/** @} */
