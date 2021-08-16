#include <string.h>
#include <math.h>

#include "embUnit.h"
#include "ztimer.h"
#include "current_time.h"
#include "time_ble_pkt.h"

static current_time_hook_t _post_hook;
static current_time_hook_t _pre_hook;
static time_update_cb_t _time_update_cb = NULL;
static volatile uint8_t calls = 0;

void desire_ble_set_time_update_cb(time_update_cb_t cb)
{
    _time_update_cb = cb;
}

static void _should_be_called(int32_t offset, void *arg)
{
    (void) offset;
    (void) arg;
    calls++;
    TEST_ASSERT(1);
}

static void _should_not_be_called(int32_t offset, void *arg)
{
    (void) offset;
    (void) arg;
    TEST_ASSERT_MESSAGE(0, "Callback should never executed");
}

static void setUp(void)
{
    /* setup */
    ztimer_init();
    current_time_init();
    calls = 0;
}

static void tearDown(void)
{
    /* finalize */
}

static void tests_current_time_early(void)
{
    current_time_ble_adv_payload_t payload;

    current_time_hook_init(&_pre_hook, _should_be_called, NULL);
    current_time_hook_init(&_post_hook, _should_be_called, NULL);
    current_time_add_pre_cb(&_pre_hook);
    current_time_add_pre_cb(&_post_hook);
    TEST_ASSERT_EQUAL_INT(0, calls);
    current_time_ble_adv_init(&payload, ztimer_now(ZTIMER_EPOCH) - 2 * CONFIG_CURRENT_TIME_RANGE_S);
    _time_update_cb(&payload);
    TEST_ASSERT_EQUAL_INT(2, calls);
}

static void tests_current_time_late(void)
{
    current_time_ble_adv_payload_t payload;

    current_time_hook_init(&_pre_hook, _should_be_called, NULL);
    current_time_hook_init(&_post_hook, _should_be_called, NULL);
    current_time_add_pre_cb(&_pre_hook);
    current_time_add_pre_cb(&_post_hook);
    TEST_ASSERT_EQUAL_INT(0, calls);
    current_time_ble_adv_init(&payload, ztimer_now(ZTIMER_EPOCH) + 2 * CONFIG_CURRENT_TIME_RANGE_S);
    _time_update_cb(&payload);
    TEST_ASSERT_EQUAL_INT(2, calls);
}

static void tests_current_time_in_range(void)
{
    current_time_ble_adv_payload_t payload;

    current_time_hook_init(&_pre_hook, _should_not_be_called, NULL);
    current_time_hook_init(&_post_hook, _should_not_be_called, NULL);
    current_time_add_pre_cb(&_pre_hook);
    current_time_add_pre_cb(&_post_hook);
    TEST_ASSERT_EQUAL_INT(0, calls);
    current_time_ble_adv_init(&payload, ztimer_now(ZTIMER_EPOCH) + CONFIG_CURRENT_TIME_RANGE_S / 2);
    _time_update_cb(&payload);
    TEST_ASSERT_EQUAL_INT(0, calls);
}

Test *tests_current_time_all(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(tests_current_time_early),
        new_TestFixture(tests_current_time_late),
        new_TestFixture(tests_current_time_in_range),
    };

    EMB_UNIT_TESTCALLER(current_time_tests, setUp, tearDown, fixtures);
    return (Test *)&current_time_tests;
}

void tests_current_time(void)
{
    TESTS_RUN(tests_current_time_all());
}
