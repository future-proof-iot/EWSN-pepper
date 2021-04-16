#include <string.h>
#include <math.h>
#include <stdio.h>
#include "timex.h"
#include "ztimer.h"
#include "random.h"

#define MAX_MSG_PER_WINDOW          120
#define WINDOWS_PER_EPOCH            15
#define ENCOUNTERS                  100

static int rssi[MAX_MSG_PER_WINDOW];

static void _bench(void)
{
    float value = 0;
    for (uint16_t i = 0; i < MAX_MSG_PER_WINDOW; i++) {
        value += pow(10, (float) rssi[i] / 10);
    }
    value = 10 * log10f(value / MAX_MSG_PER_WINDOW);
}

int main(void)
{
    uint32_t start;
    uint32_t end;
    for (int i = 0; i < MAX_MSG_PER_WINDOW; i++) {
        rssi[i] = -1 * ((int) random_uint32_range(0, 60));
    }
    puts("Fading test");

    start = ztimer_now(ZTIMER_MSEC);
    _bench();
    end = ztimer_now(ZTIMER_MSEC);
    printf("%d messages:   %05"PRIu32" [ms]\n", MAX_MSG_PER_WINDOW, end - start);

    start = ztimer_now(ZTIMER_MSEC);
    for (int j = 0; j < WINDOWS_PER_EPOCH; j++) {
        _bench();
    }
    end = ztimer_now(ZTIMER_MSEC);
    printf(" %d windows:    %05"PRIu32" [ms]\n", WINDOWS_PER_EPOCH, end - start);

    start = ztimer_now(ZTIMER_MSEC);
    for (int j = 0; j < WINDOWS_PER_EPOCH * ENCOUNTERS; j++) {
        _bench();
    }
    end = ztimer_now(ZTIMER_MSEC);
    printf("%d encounters: %05"PRIu32" [ms]\n", ENCOUNTERS, end - start);

    return 0;
}
