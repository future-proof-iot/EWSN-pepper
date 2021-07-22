#include <string.h>
#include <math.h>

#include "embUnit.h"
#include "state_manager.h"

static uint8_t expected_infected_cbor[] = { 0xd9, 0xca, 0xfa, 0x81, 0xf5 };

static void setUp(void)
{
    /* setup */
    state_manager_init();
}

static void tearDown(void)
{
    /* finalize */
}

static void tests_state_manager_serialize_cbor(void)
{
    uint8_t buf[8];

    state_manager_set_infected_status(true);
    TEST_ASSERT_EQUAL_INT(
        sizeof(expected_infected_cbor),
        state_manager_infected_serialize_cbor(buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(0,
                          memcmp(expected_infected_cbor, buf,
                                 sizeof(expected_infected_cbor)));
}

static void tests_state_manager_esr_load_cbor(void)
{
    uint8_t buf[] = { 0xd9, 0xca, 0xff, 0x81, 0xf5 };

    TEST_ASSERT(state_manager_get_esr() == false);
    TEST_ASSERT_EQUAL_INT(0, state_manager_esr_load_cbor(buf, sizeof(buf)));
    TEST_ASSERT(state_manager_get_esr() == true);
}

Test *tests_state_manager_all(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(tests_state_manager_esr_load_cbor),
        new_TestFixture(tests_state_manager_serialize_cbor),
    };

    EMB_UNIT_TESTCALLER(rdl_tests, setUp, tearDown, fixtures);
    return (Test *)&rdl_tests;
}

void tests_state_manager(void)
{
    TESTS_RUN(tests_state_manager_all());
}
