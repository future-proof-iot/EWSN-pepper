#include <string.h>
#include <math.h>

#include "embUnit.h"
#include "rdl_window.h"
#include "ztimer.h"

#define TEST_VALUES_NUMOF       10

static uint16_t data_ts[TEST_VALUES_NUMOF] = {
    WINDOW_STEP_S - 1,      /* win 0 */
    WINDOW_STEP_S * 2 - 3,  /* win 0 & 1 */
    WINDOW_STEP_S * 2 - 2,  /* win 0 & 1 */
    WINDOW_STEP_S * 2 - 1,  /* win 0 & 1 */
    WINDOW_STEP_S * 5,      /* win 4 and 5 */
    WINDOW_STEP_S * 6 - 4,  /* win 4 and 5 */
    WINDOW_STEP_S * 6 - 3,  /* win 4 and 5  */
    WINDOW_STEP_S * 6 - 2,  /* win 4 and 5 */
    WINDOW_STEP_S * 10,     /* win 9 and 10 */
    WINDOW_STEP_S * 11,     /* win 10 and 11 */
};

static float data_rssi[TEST_VALUES_NUMOF] = {
    -80, -70, -50, -10, -20, -30, -40, -75, -25, -35,
};

static int expected_samples[WINDOWS_PER_EPOCH] = {
    4, 3, 0, 0, 4, 4, 0, 0, 0, 1, 2, 1, 0, 0, 0
};

static float expected_avg[WINDOWS_PER_EPOCH] = {
    -16.02016, -14.77077, 000.00000, 000.00000,
    -25.56735, -25.56735, 000.00000, 000.00000,
    000.00000, -25.00000, -27.59637, -35.00000,
    000.00000, 000.00000, 000.00000
};

static float _2_dec_round(float value)
{
    return round(100 * value) / 100;
}

static void setUp(void)
{
    /* setup */
}

static void tearDown(void)
{
    /* finalize */
}

static void tests_rdl_windows(void)
{
    rdl_windows_t windows;

    rdl_windows_init(&windows);

    for (uint8_t i = 0; i < TEST_VALUES_NUMOF; i++) {
        rdl_windows_update(&windows, data_rssi[i], data_ts[i]);
    }
    rdl_windows_finalize(&windows);
    for (uint8_t i = 0; i < WINDOWS_PER_EPOCH; i++) {
        TEST_ASSERT(windows.wins[i].samples == expected_samples[i]);
        TEST_ASSERT(_2_dec_round(windows.wins[i].avg) ==
                    _2_dec_round(expected_avg[i]));
    }
}

Test *tests_edl(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(tests_rdl_windows),
    };

    EMB_UNIT_TESTCALLER(edl_tests, setUp, tearDown, fixtures);
    return (Test *)&edl_tests;
}

int main(void)
{
    TESTS_START();
    TESTS_RUN(tests_edl());
    TESTS_END();

    return 0;
}
