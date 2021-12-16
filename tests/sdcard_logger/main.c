#include <fcntl.h>
#include "kernel_defines.h"
#include "epoch.h"
#include "ztimer.h"
#include "pepper.h"
#include "storage.h"

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
    uint32_t start = ztimer_now(ZTIMER_MSEC);

    if (storage_log("sys/log/epoch.txt", buffer, len - 1 )) {
        puts("[FAILED]");
        puts("ERROR, failed to log to storage");
        return -1;
    }

    uint32_t stop = ztimer_now(ZTIMER_MSEC);

    puts("[SUCCESS]");
    printf("exectime: %" PRIu32 "\n", stop - start);
    return 0;
}
