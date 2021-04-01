#include <string.h>

#include "embUnit.h"
#include "crypto_manager.h"
#include "random.h"

static crypto_manager_keys_t keys;

static void setUp(void)
{
    random_init(0);
    memset(&keys, 0, sizeof(crypto_manager_keys_t));
}

static void tearDown(void)
{
    /* Finalize */
}

static void test_crypto_manager_gen_keypair(void)
{
    crypto_manager_keys_t empty_keys;
    memset(&empty_keys, 0, sizeof(crypto_manager_keys_t));
    TEST_ASSERT(crypto_manager_gen_keypair(&keys) == 0);
    TEST_ASSERT(memcmp(&empty_keys, &keys, sizeof(crypto_manager_keys_t)));
}

static void test_crypto_manager_gen_pet(void)
{
    uint8_t pet[PET_SIZE];
    const uint8_t prefix = 0;
    crypto_manager_keys_t pub_keys;
    TEST_ASSERT(crypto_manager_gen_keypair(&keys) == 0);
    TEST_ASSERT(crypto_manager_gen_keypair(&pub_keys) == 0);
    TEST_ASSERT(crypto_manager_gen_pet(&keys, pub_keys.pk, prefix, pet) == 0);
}

Test *tests_crypto_manager(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_crypto_manager_gen_keypair),
        new_TestFixture(test_crypto_manager_gen_pet)
    };

    EMB_UNIT_TESTCALLER(crypto_manager_tests, setUp, tearDown, fixtures);
    return (Test*)&crypto_manager_tests;
}

int main(void)
{
    TESTS_START();
    TESTS_RUN(tests_crypto_manager());
    TESTS_END();

    return 0;
}
