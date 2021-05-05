#include <string.h>

#include "embUnit.h"
#include "crypto_manager.h"
#include "random.h"
#include "base64.h"

#define ENABLE_DEBUG    0
#include "debug.h"

static crypto_manager_keys_t desire_keys_bob;
static crypto_manager_keys_t desire_keys_alice;
static uint8_t expected_pet_1[C25519_KEY_SIZE];
static uint8_t expected_pet_2[C25519_KEY_SIZE];
static uint8_t expected_secret[C25519_KEY_SIZE];

static const char b64_pk_bob[] =
    "wsPGv9bw2Q0mLNR8Q+TA5q4e6GQzRIiPxl5gXtrsZi8=";
static const char b64_sk_bob[] =
    "IAB2ptLioKUQA+T4SH8AAAAAAAAAAAAAEAPk+Eh/AEA=";
static const char b64_pk_alice[] =
    "l9OSbVr/VD2XhyCbVdvaRoIjhWxCuW1iWaQ0GeHkLzo=";
static const char b64_sk_alice[] =
    "OBgpK9a1/XWQruL4SH8AAAAAAAAAAAAAAMfgNUl/AEA=";
static const char b64_expected_pet_1[] =
    "KqO9fF5bvHtJFh6uWSDBnaO4JZu6hi/AJTjLbSyPklE=";
static const char b64_expected_pet_2[] =
    "iscm1Ih0+xfKL38bF/jONgeGkhqSKaWyaokgxGiT+1U=";
static const char b64_expected_secret[] =
    "1IxkgxMHIKvy0vAvgsdVs8r6LpNFeDAGRzAR0NQA0Fo=";


static void _b64_decode(const char *str_b64, uint8_t *out)
{
    uint8_t buf[45];
    size_t len = sizeof(buf);

    base64_decode(str_b64, strlen(str_b64), buf, &len);
    TEST_ASSERT_EQUAL_INT(C25519_KEY_SIZE, len);
    memcpy(out, buf, len);
}

static void _b64_decode_test_vectors(void)
{
    _b64_decode(b64_pk_bob, desire_keys_bob.pk);
    _b64_decode(b64_sk_bob, desire_keys_bob.sk);
    _b64_decode(b64_pk_alice, desire_keys_alice.pk);
    _b64_decode(b64_sk_alice, desire_keys_alice.sk);
    _b64_decode(b64_expected_pet_1, expected_pet_1);
    _b64_decode(b64_expected_pet_2, expected_pet_2);
    _b64_decode(b64_expected_secret, expected_secret);
}

static void setUp(void)
{
    random_init(0);
    _b64_decode_test_vectors();
}

static void tearDown(void)
{
    /* Finalize */
}
static void test_crypto_manager_gen_keypair(void)
{
    crypto_manager_keys_t keys;
    crypto_manager_keys_t empty_keys;

    memset(&keys, 0, sizeof(crypto_manager_keys_t));
    memset(&empty_keys, 0, sizeof(crypto_manager_keys_t));
    TEST_ASSERT(crypto_manager_gen_keypair(&keys) == 0);
    /* not much to test here just that the keys are set */
    TEST_ASSERT(memcmp(&empty_keys, &keys, sizeof(crypto_manager_keys_t)));
}

static void test_crypto_manager_shared_secret(void)
{
    uint8_t secret[C25519_KEY_SIZE] = { 0 };

    crypto_manager_shared_secret(desire_keys_alice.sk, desire_keys_bob.pk,
                                 secret);
    TEST_ASSERT(memcmp(secret, expected_secret, C25519_KEY_SIZE) == 0);
    memset(secret, 0, sizeof(secret));
    crypto_manager_shared_secret(desire_keys_bob.sk, desire_keys_alice.pk,
                                 secret);
    TEST_ASSERT(memcmp(secret, expected_secret, C25519_KEY_SIZE) == 0);
}

static void test_crypto_manager_gen_pet(void)
{
    uint8_t pet_1[PET_SIZE] = { 0 };
    uint8_t pet_2[PET_SIZE] = { 0 };

    TEST_ASSERT(crypto_manager_gen_pet(&desire_keys_bob, desire_keys_alice.pk,
                                       0x01, pet_1) == 0);
    TEST_ASSERT(crypto_manager_gen_pet(&desire_keys_bob, desire_keys_alice.pk,
                                       0x02, pet_2) == 0);
    TEST_ASSERT(memcmp(pet_1, expected_pet_1, PET_SIZE) == 0);
    TEST_ASSERT(memcmp(pet_2, expected_pet_2, PET_SIZE) == 0);
    TEST_ASSERT(crypto_manager_gen_pet(&desire_keys_alice, desire_keys_bob.pk,
                                       0x01, pet_1) == 0);
    TEST_ASSERT(crypto_manager_gen_pet(&desire_keys_alice, desire_keys_bob.pk,
                                       0x02, pet_2) == 0);
    TEST_ASSERT(memcmp(pet_1, expected_pet_1, PET_SIZE) == 0);
    TEST_ASSERT(memcmp(pet_2, expected_pet_2, PET_SIZE) == 0);
}

static void test_crypto_manager_gen_pets(void)
{
    pet_t pet_bob = { 0 };
    pet_t pet_alice = { 0 };

    TEST_ASSERT(crypto_manager_gen_pets(&desire_keys_bob, desire_keys_alice.pk,
                                        &pet_bob) == 0);
    TEST_ASSERT(crypto_manager_gen_pets(&desire_keys_alice, desire_keys_bob.pk,
                                        &pet_alice) == 0);
    TEST_ASSERT(memcmp(pet_bob.et, expected_pet_1, PET_SIZE) == 0);
    TEST_ASSERT(memcmp(pet_bob.rt, expected_pet_2, PET_SIZE) == 0);
    TEST_ASSERT(memcmp(pet_alice.rt, expected_pet_1, PET_SIZE) == 0);
    TEST_ASSERT(memcmp(pet_alice.et, expected_pet_2, PET_SIZE) == 0);
}

Test *tests_crypto_manager_all(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_crypto_manager_gen_keypair),
        new_TestFixture(test_crypto_manager_shared_secret),
        new_TestFixture(test_crypto_manager_gen_pet),
        new_TestFixture(test_crypto_manager_gen_pets)
    };

    EMB_UNIT_TESTCALLER(crypto_manager_tests, setUp, tearDown, fixtures);
    return (Test *)&crypto_manager_tests;
}

void tests_crypto_manager(void)
{
    TESTS_RUN(tests_crypto_manager_all());
}
