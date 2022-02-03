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

static float _bench(void)
{
    float value = 0;

    for (uint16_t i = 0; i < MAX_MSG_PER_WINDOW; i++) {
        value += pow(10, (float)rssi[i] / 10);
    }
    value = 10 * log10f(value / MAX_MSG_PER_WINDOW);

    return value;
}

static double _rssi_avg(void)
{
    double value = 0;

    for (uint16_t i = 0; i < MAX_MSG_PER_WINDOW; i++) {
        value += pow(10.0, (double)rssi[i] / 10);
    }
    value = 10.0 * log10(value / MAX_MSG_PER_WINDOW);

    return value;
}

void test_fading(void)
{
    uint32_t start;
    uint32_t end;

    puts("Fading test");

    start = ztimer_now(ZTIMER_MSEC);
    _bench();
    end = ztimer_now(ZTIMER_MSEC);
    printf("%d messages:   %05" PRIu32 " [ms]\n", MAX_MSG_PER_WINDOW,
           end - start);

    start = ztimer_now(ZTIMER_MSEC);
    for (int j = 0; j < WINDOWS_PER_EPOCH; j++) {
        _bench();
    }
    end = ztimer_now(ZTIMER_MSEC);
    printf(" %d windows:    %05" PRIu32 " [ms]\n", WINDOWS_PER_EPOCH,
           end - start);

    start = ztimer_now(ZTIMER_MSEC);
    for (int j = 0; j < WINDOWS_PER_EPOCH * ENCOUNTERS; j++) {
        _bench();
    }
    end = ztimer_now(ZTIMER_MSEC);
    printf("%d encounters: %05" PRIu32 " [ms]\n", ENCOUNTERS, end - start);
}


void test_rssi_models(void)
{
    #define RSSI_BASE (-62) //Aveerage rssi value in dBm
    #define MODEL(rssi) ((uint16_t)(0.0066256 * pow(0.8546459, rssi)))

    /* Generate random RSSI around a mean value */
    for (int i = 0; i < MAX_MSG_PER_WINDOW; i++) {
        rssi[i] = RSSI_BASE + (int)random_uint32_range(0, 10) -  (int)random_uint32_range(0, 10);
    }

    float rss_f = _bench();
    double rss = _rssi_avg();

    printf("rss (float) = %.8f dBm -> %d cm\n", rss_f, MODEL(rss_f));
    printf("rss (double) = %.8lf dBm -> %d cm\n", rss, MODEL(rss));
}

int main(void)
{
    /* Generate random RSSI */
    for (int i = 0; i < MAX_MSG_PER_WINDOW; i++) {
        rssi[i] = -1 * ((int)random_uint32_range(0, 60));
    }

    test_fading();
    test_rssi_models();

    return 0;
}
