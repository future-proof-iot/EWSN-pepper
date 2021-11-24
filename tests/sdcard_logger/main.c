
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

    storage_init();
    random_epoch(&_epoch_data);
    len = contact_data_serialize_all_json(&_epoch_data, buffer, sizeof(buffer), "toto");
    uint32_t start = ztimer_now(ZTIMER_MSEC);

    storage_log("sys/log/epoch.txt", buffer, len - 1 );
    uint32_t stop = ztimer_now(ZTIMER_MSEC);

    printf("exectime: %" PRIu32 "\n", stop - start);
    return 0;
}
