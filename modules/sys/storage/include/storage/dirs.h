/*
 * Copyright (C) 2018 Koen Zandberg <koen@bergzand.net>
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_storage Storage Directories
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

#ifndef STORAGE_DIRS_H
#define STORAGE_DIRS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _storage_dir storage_dir_t;

struct _storage_dir {
    const storage_dir_t *subdirs;
    const char *name;
};

int storage_dirs_create_hier(const storage_dir_t *hierarchy, const char *prefix);
int storage_dirs_create_sys_hier(void);
#ifdef __cplusplus
}
#endif

#endif /* STORAGE_DIRS_H */
