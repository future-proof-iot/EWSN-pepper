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


/***   RSS-based distance model  ****/
#ifndef MODEL_ALPHA
#define MODEL_ALPHA (0.007820346529883561)
#endif
#ifndef MODEL_BETA
#define MODEL_BETA (0.8549878593446547)
#endif
#ifndef MODEL_PLE
#define MODEL_PLE (1.47)
#endif
#ifndef MODEL_R0
#define MODEL_R0 (-60.36)
#endif

static struct {
    /* d(rssi) = alpha*beta^(rssi) parameters */
    double alpha;
    double beta;
    /*  rssi(d) = r0 - 10*ple*log10(d)  parameters */
    double r0;
    double ple;
} rss_model = { MODEL_ALPHA, MODEL_BETA, MODEL_R0, MODEL_PLE };

static void model_print(void)
{
    printf("Rss model : alpha = %f cm, beta = %f, r0 = %f dBm, ple = %f\n", rss_model.alpha,
           rss_model.beta, rss_model.r0, rss_model.ple);
}

static uint16_t model_rssi_to_cm(float rssi)
{
    return (uint16_t)(rss_model.alpha * pow(rss_model.beta, rssi));  // note alpha coef fixes the cm unit
}

static int8_t model_cm_to_rssi(uint16_t d_cm)
{
    return (int8_t)round(rss_model.r0 - 10.0 * rss_model.ple * log10(d_cm / 100));
}


void test_rssi_models(int8_t rssi_mean)
{
    /* Generate random RSSI around a mean value (uniform distribution) */
    printf("rssi samples = [");
    for (int i = 0; i < MAX_MSG_PER_WINDOW; i++) {
        rssi[i] = rssi_mean + (int)random_uint32_range(0, 5) -  (int)random_uint32_range(0, 5);
        printf(" %d", rssi[i]);
    }
    printf(" ]\n");

    /* Generate average rssi in float and double */
    float rss_f = _bench();
    double rss = _rssi_avg();

    printf("rss (float) = %.8f dBm -> %d cm\n", rss_f, model_rssi_to_cm(rss_f));
    printf("rss (double) = %.8lf dBm -> %d cm\n", rss, model_rssi_to_cm(rss));
}

int main(void)
{
    /* Generate random RSSI */
    for (int i = 0; i < MAX_MSG_PER_WINDOW; i++) {
        rssi[i] = -1 * ((int)random_uint32_range(0, 60));
    }

    test_fading();

    /* Test RSSI models */
    static int dist_cm[4] = { 50, 120, 200, 300 };

    model_print();
    for (int i = 0; i < 4; i++) {
        int8_t rssi = model_cm_to_rssi(dist_cm[i]);
        printf("------------------\n");
        printf(
            "Testing for rss mean = %d dBm at distance d = %d cm [Inverse noise-free distance = %d cm]\n", rssi,
            dist_cm[i], model_rssi_to_cm(rssi));
        test_rssi_models(rssi);
        printf("------------------\n");
    }
    

    return 0;
}
