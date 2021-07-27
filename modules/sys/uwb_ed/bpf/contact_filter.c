#include <stdint.h>
#include "uwb_ed_shared.h"

int contact_filter(bpf_uwb_ed_ctx_t *ctx)
{
    if (ctx->distance <= MAX_DISTANCE_CM && ctx->time >= MIN_EXPOSURE_TIME_S) {
        return 1;
    }
    return 0;
}
