/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_storage
 * @{
 *
 * @file
 * @brief       Storage implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <stdint.h>
#include <errno.h>
#include <fcntl.h>

#include "fs/fatfs.h"
#include "vfs.h"
#include "board.h"

#include "storage.h"
#include "storage/dirs.h"
#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

static fatfs_desc_t fs_desc;
mtd_dev_t *fatfs_mtd_devs[FF_VOLUMES];

static vfs_mount_t _storage_fs_mount = {
    .fs = &fatfs_file_system,
    .mount_point = STORAGE_FS_MOUNT_POINT,
    .private_data = &fs_desc,
};

/* Configure MTD device for SD card if none is provided */
#if !defined(MTD_0) && MODULE_MTD_SDCARD
#include "mtd_sdcard.h"
#include "sdcard_spi.h"
#include "sdcard_spi_params.h"

#define SDCARD_SPI_NUM ARRAY_SIZE(sdcard_spi_params)

/* SD card devices are provided by drivers/sdcard_spi/sdcard_spi.c */
extern sdcard_spi_t sdcard_spi_devs[SDCARD_SPI_NUM];

/* Configure MTD device for the first SD card */
static mtd_sdcard_t mtd_sdcard_dev = {
    .base = {
        .driver = &mtd_sdcard_driver
    },
    .sd_card = &sdcard_spi_devs[0],
    .params = &sdcard_spi_params[0],
};
static mtd_dev_t *mtd0 = (mtd_dev_t *)&mtd_sdcard_dev;
#define MTD_0 mtd0
#endif

static int _storage_prepare(vfs_mount_t *fs)
{
    LOG_INFO("[fs]: Initializing file system mount %s\n", fs->mount_point);
    if (IS_ACTIVE(CONFIG_STORAGE_FS_FORCE_FORMAT)) {
        assert(vfs_format(fs) == 0);
    }
    int res = vfs_mount(fs);

    if (res < 0) {
        LOG_ERROR("[fs]: Unable to mount filesystem %s: %d\n", fs->mount_point,
                  res);
        res = vfs_format(fs);
        if (res < 0) {
            LOG_ERROR("[fs]: Unable to format filesystem %s: %d\n",
                      fs->mount_point, res);
        }
        return vfs_mount(fs);
    }
    return res;
}

int storage_init(void)
{
    if(IS_ACTIVE(MTD_0)) {
        fatfs_mtd_devs[fs_desc.vol_idx] = MTD_0;
        assert(_storage_prepare(&_storage_fs_mount) == 0);
        storage_dirs_create_sys_hier();
        return 0;
    }
    else {
        return -1;
    }
}

int storage_log(const char* path, uint8_t *buffer, size_t len)
{
    if(IS_ACTIVE(MTD_0)) {
        char fullpath[VFS_NAME_MAX + 1];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", STORAGE_FS_MOUNT_POINT, path);

        int fd = vfs_open(fullpath, O_WRONLY | O_APPEND | O_CREAT, 0);

        if (fd < 0) {
            printf("[fs]: error while trying to create %s\n", fullpath);
            return 1;
        }
        if (vfs_write(fd, buffer, len) != (ssize_t)len) {
            LOG_ERROR("[fs]: error while writing");
        }
        vfs_close(fd);
        return 0;
    }
    (void)path;
    (void)buffer;
    (void)len;
    return -1;
}
