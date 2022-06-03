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
#include <assert.h>
#include <fcntl.h>

#include "vfs.h"
#include "board.h"

#include "storage.h"
#include "storage/dirs.h"
#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_DEBUG
#endif
#include "log.h"

/* Configure MTD device for SD card if none is provided */
#if !defined(MTD_0) && MODULE_MTD_SDCARD
#include "mtd_sdcard.h"
#include "sdcard_spi.h"
#include "sdcard_spi_params.h"

#define SDCARD_SPI_NUM ARRAY_SIZE(sdcard_spi_params)

/* SD card devices are provided by drivers/sdcard_spi/sdcard_spi.c */
extern sdcard_spi_t sdcard_spi_devs[SDCARD_SPI_NUM];

/* Configure MTD device for the first SD card */
static mtd_sdcard_t _mtd_sdcard = {
    .base = {
        .driver = &mtd_sdcard_driver
    },
    .sd_card = &sdcard_spi_devs[0],
    .params = &sdcard_spi_params[0],
};
static mtd_dev_t *_mtd0 = (mtd_dev_t *)&_mtd_sdcard;
#define MTD_0     _mtd0

#ifdef MODULE_VFS_DEFAULT
#include "vfs_default.h"
#if defined(MODULE_LITTLEFS)
VFS_AUTO_MOUNT(littlefs, VFS_MTD(_mtd_sdcard), VFS_DEFAULT_NVM(0), 0);
/* littlefs2 support */
#elif defined(MODULE_LITTLEFS2)
VFS_AUTO_MOUNT(littlefs2, VFS_MTD(_mtd_sdcard_mtd0), VFS_DEFAULT_NVM(0), 0);
/* spiffs support */
#elif defined(MODULE_SPIFFS)
VFS_AUTO_MOUNT(spiffs, VFS_MTD(_mtd_sdcard_mtd0), VFS_DEFAULT_NVM(0), 0);
/* FAT support */
#elif defined(MODULE_FATFS_VFS)
VFS_AUTO_MOUNT(fatfs, VFS_MTD(_mtd_sdcard_mtd0), VFS_DEFAULT_NVM(0), 0);
#endif
#endif
#endif

int storage_init(void)
{
    if (MTD_0) {
        int res = vfs_mount_by_path(VFS_STORAGE_DATA);
        if ( res != 0 && res != -EBUSY) {
            LOG_ERROR("[fs]: ERROR, failed to setup %s res=(%d)\n", VFS_STORAGE_DATA, res);
            return -1;
        }
        return storage_dirs_create_sys_hier();
    }
    else {
        return -1;
    }
}

int storage_deinit(void)
{
    if (MTD_0) {
        return vfs_unmount_by_path(VFS_STORAGE_DATA);
    }
    else {
        return -1;
    }
}

int storage_log(const char *path, uint8_t *buffer, size_t len)
{
    if (MTD_0) {
        int fd = vfs_open(path, O_WRONLY | O_APPEND | O_CREAT, 0);

        if (fd < 0) {
            LOG_ERROR("[fs]: error while trying to create %s\n", path);
            return 1;
        }
        if (vfs_write(fd, buffer, len) != (ssize_t)len) {
            LOG_ERROR("[fs]: error while writing\n");
        }
        vfs_close(fd);
        return 0;
    }
    (void)buffer;
    (void)len;
    return -1;
}
