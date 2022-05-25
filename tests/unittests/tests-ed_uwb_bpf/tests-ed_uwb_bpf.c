#include <string.h>
#include <math.h>

#include "embUnit.h"
#include "ed.h"
#include "femtocontainer/femtocontainer.h"

static void setUp(void)
{
    /* setup */
}

static void tearDown(void)
{
    /* Finalize */
}

static void test_ed_uwb_bpf_finish(void)
{
    ed_t ed;
    ed_init(&ed, 0x01);
    ed.uwb.seen_first_s = 0;
    ed.uwb.seen_last_s = MIN_EXPOSURE_TIME_S - 1;
    TEST_ASSERT(ed_uwb_bpf_finish(&ed) == false);
    ed.uwb.seen_last_s = MIN_EXPOSURE_TIME_S;
    ed.uwb.req_count = MIN_REQUEST_COUNT - 1;
    TEST_ASSERT(ed_uwb_bpf_finish(&ed) == false);
    ed.uwb.req_count = 4;
    ed.uwb.cumulative_d_cm = (MAX_DISTANCE_CM + 1) * 4;
    TEST_ASSERT(ed_uwb_bpf_finish(&ed) == false);
    ed.uwb.cumulative_d_cm = (MAX_DISTANCE_CM - 1) * 4;
    TEST_ASSERT(ed_uwb_bpf_finish(&ed) == true);
    TEST_ASSERT(ed.uwb.valid == true);
}

Test *tests_ed_uwb_bpf_all(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_ed_uwb_bpf_finish),
    };

    EMB_UNIT_TESTCALLER(ed_tests, setUp, tearDown, fixtures);
    return (Test *)&ed_tests;
}

void tests_ed_uwb_bpf(void)
{
    TESTS_RUN(tests_ed_uwb_bpf_all());
}

