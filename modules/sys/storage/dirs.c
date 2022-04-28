/*
 * Copyright (C) 2018 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdint.h>
#include <errno.h>
#include <string.h>

#include "vfs.h"
#include "storage.h"
#include "storage/dirs.h"
#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

static const storage_dir_t sys_hier[] = {
    {
        .name = "/log",
    },
    { NULL, NULL },
};

static ssize_t _add_dir_name(char *buf, size_t buf_len, const char *name)
{
    size_t name_len = strlen(name); /* Possibly unsafe use of strlen */
    if (name_len == 0) {
        buf[0] = '/';
        name_len = 1;
    }
    else if (name_len >= buf_len) {
        return -2;
    }
    else {
        if (name[0] != '/') {
            return -1;
        }
        strcpy(buf, name);
    }
    return name_len;
}

static int _storage_dir_iter(const storage_dir_t *hier, char *name_buf, size_t len, size_t offset)
{
    while(hier->name) {
        ssize_t new_offset = _add_dir_name(&name_buf[offset], len - offset, hier->name);
        if (new_offset < 0) {
            return -1;
        }
        int res = vfs_mkdir(name_buf, 0);
        if (res < 0) {
            if (res != -EEXIST) {
                LOG_ERROR("[dirs]: unable to create dir %s: %d\n", name_buf, res);
            }
        }
        if (hier->subdirs) {
            _storage_dir_iter(hier->subdirs, name_buf, len, offset + new_offset);
        }
        hier++;
    }
    return 0;
}

int storage_dirs_create_hier(const storage_dir_t *hierarchy, const char *prefix)
{
    char name_buf[64];
    ssize_t prefix_len = _add_dir_name(name_buf, sizeof(name_buf), prefix);
    if (prefix_len < 0) {
        return prefix_len;
    }
    return _storage_dir_iter(hierarchy, name_buf, sizeof(name_buf), prefix_len);
}

int storage_dirs_create_sys_hier(void)
{
    return storage_dirs_create_hier(sys_hier, VFS_STORAGE_DATA);
}
