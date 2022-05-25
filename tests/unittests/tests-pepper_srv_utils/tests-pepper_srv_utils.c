#include <string.h>
#include <math.h>

#include "embUnit.h"
#include "pepper_srv_utils.h"

static uint8_t expected_infected_cbor[] = { 0xd9, 0xca, 0xfa, 0x81, 0xf5 };

static void setUp(void)
{
    /* setup */
}

static void tearDown(void)
{
    /* finalize */
}

static void tests_pepper_srv_serialize_cbor(void)
{
    uint8_t buf[8];

    TEST_ASSERT_EQUAL_INT(
        sizeof(expected_infected_cbor),
        pepper_srv_infected_serialize_cbor(buf, sizeof(buf), true));
    TEST_ASSERT_EQUAL_INT(0, memcmp(expected_infected_cbor, buf,
                                    sizeof(expected_infected_cbor)));
}

static void tests_pepper_srv_esr_load_cbor(void)
{
    uint8_t buf[] = { 0xd9, 0xca, 0xff, 0x81, 0xf5 };
    bool status = false;

    TEST_ASSERT_EQUAL_INT(0, pepper_srv_esr_load_cbor(buf, sizeof(buf), &status));
    TEST_ASSERT(status == true);
}

Test *tests_pepper_srv_utils_all(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(tests_pepper_srv_esr_load_cbor),
        new_TestFixture(tests_pepper_srv_serialize_cbor),
    };

    EMB_UNIT_TESTCALLER(rdl_tests, setUp, tearDown, fixtures);
    return (Test *)&rdl_tests;
}

void tests_pepper_srv_utils(void)
{
    TESTS_RUN(tests_pepper_srv_utils_all());
}
