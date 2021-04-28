#include <string.h>
#include <math.h>

#include "embUnit.h"
#include "edl.h"
#include "rdl_window.h"
#include "ztimer.h"

#define EDL_TEST_VALUES_NUMOF       10

static edl_list_t list;
static edl_t data[EDL_TEST_VALUES_NUMOF];

static uint32_t data_ts[EDL_TEST_VALUES_NUMOF] = {
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

static int data_rssi[EDL_TEST_VALUES_NUMOF] = {
    -80,
    -70,
    -50,
    -10,
    -20,
    -30,
    -40,
    -75,
    -25,
    -35,
};

static int expected_samples[WINDOWS_PER_EPOCH] = {
    4,
    3,
    0,
    0,
    4,
    4,
    0,
    0,
    0,
    1,
    2,
    1,
    0,
    0,
    0
};

static float expected_avg[WINDOWS_PER_EPOCH] = {
    -16.02016,
    -14.77077,
     0.0,
     0.0,
     -25.56735,
     -25.56735,
     0.0,
     0.0,
     0.0,
     -25.00,
     -27.59637,
     -35.00,
     0.0,
     0.0,
     0.0
};

static const uint8_t ebid[] = {
    0x20, 0x21, 0x22, 0x23, 0x24,
    0x25, 0x26, 0x27, 0x28, 0x29,
    0x20, 0x2e, 0x10, 0x11, 0x12,
    0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x20, 0xfe, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15,
    0x16, 0x17
};

static float _2_dec_round(float value)
{
    return round(100 * value) / 100;
}

static void setUp(void)
{
    edl_list_init(&list, (void*) ebid);
    for (uint8_t i = 0; i < EDL_TEST_VALUES_NUMOF; i++) {
        edl_init_rssi(&data[i], data_rssi[i], data_ts[i]);
        edl_add(&list, &data[i]);
    }
}

static void tearDown(void)
{
    /* Finalize */
}

static void tests_rdl_windows_from_edl_list(void)
{
    rdl_windows_t windows;
    rdl_windows_from_edl_list(&list, 0, &windows);
    for (uint8_t i = 0; i < WINDOWS_PER_EPOCH; i++) {
        TEST_ASSERT(windows.wins[i].samples == expected_samples[i]);
        TEST_ASSERT(_2_dec_round(windows.wins[i].avg) == _2_dec_round(expected_avg[i]));
    }
}


Test *tests_edl(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(tests_rdl_windows_from_edl_list),
    };

    EMB_UNIT_TESTCALLER(edl_tests, setUp, tearDown, fixtures);
    return (Test*)&edl_tests;
}

int main(void)
{
    TESTS_START();
    TESTS_RUN(tests_edl());
    TESTS_END();

    return 0;
}
