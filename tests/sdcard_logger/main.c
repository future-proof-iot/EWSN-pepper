#include <fcntl.h>
#include "kernel_defines.h"
#include "epoch.h"
#include "ztimer.h"
#include "pepper.h"
#include "storage.h"
#include "vfs.h"

#ifndef ITERATIONS
#define ITERATIONS  ((uint32_t)100LU)
#endif

#define TEST_FILE_PATH      (VFS_STORAGE_DATA "/log/tmp.txt")

static epoch_data_t _epoch_data;
static uint8_t buffer[2048];

int main(void)
{
    size_t len;

    if (storage_init() != 0) {
        puts("[FAILED]");
        puts("ERROR, failed to init storage");
        return -1;
    }
    random_epoch(&_epoch_data);
    len = contact_data_serialize_all_json(&_epoch_data, buffer, sizeof(buffer), "toto");
    uint32_t start = ztimer_now(ZTIMER_USEC);

    for (uint32_t i = 0; i < ITERATIONS; i++) {
        if (storage_log(TEST_FILE_PATH, buffer, len - 1 )) {
            puts("[FAILED]");
            puts("ERROR, failed to log to storage");
            return -1;
        }
    }

    uint32_t stop = ztimer_now(ZTIMER_USEC);

    puts("[SUCCESS]");
    printf("exectime: %" PRIu32 "us / %" PRIu32 " , avg: %" PRIu32 "us\n", stop - start, ITERATIONS,
           (stop - start) / ITERATIONS);

    vfs_unlink(TEST_FILE_PATH);

    return 0;
}
