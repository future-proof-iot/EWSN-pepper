#include <stdint.h>
#include "bpf/ed.h"

int contact_filter(ed_uwb_bpf_ctx_t *ctx)
{
    if (ctx->distance <= MAX_DISTANCE_CM && \
        ctx->time >= MIN_EXPOSURE_TIME_S &&
        ctx->req_count >= MIN_REQUEST_COUNT) {
        return 1;
    }
    return 0;
}
