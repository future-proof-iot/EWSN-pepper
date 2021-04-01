#include <string.h>
#include <stdio.h>

#include "embUnit.h"
#include "crypto_manager.h"
#include "random.h"
#include "base64.h"

static crypto_manager_keys_t desire_keys;
static uint8_t expected_pet_1[32];
static uint8_t expected_pet_2[32];
static uint8_t ebid[C25519_KEY_SIZE];
static uint8_t expected_secret[C25519_KEY_SIZE];

static const char b64_pk[]   = "wsPGv9bw2Q0mLNR8Q+TA5q4e6GQzRIiPxl5gXtrsZi8=";
static const char b64_sk[]   = "IAB2ptLioKUQA+T4SH8AAAAAAAAAAAAAEAPk+Eh/AEA=";
static const char b64_ebid[] = "l9OSbVr/VD2XhyCbVdvaRoIjhWxCuW1iWaQ0GeHkLzo=";
/* matching secret key "OBgpK9a1/XWQruL4SH8AAAAAAAAAAAAAAMfgNUl/AEA=" */
static const char b64_expected_pet_1[] = "KqO9fF5bvHtJFh6uWSDBnaO4JZu6hi/AJTjLbSyPklE=";
static const char b64_expected_pet_2[] = "iscm1Ih0+xfKL38bF/jONgeGkhqSKaWyaokgxGiT+1U=";
static const char b64_secret[] = "1IxkgxMHIKvy0vAvgsdVs8r6LpNFeDAGRzAR0NQA0Fo=";

static void _b64_decode_test_vectors(void)
{
    /* the estimated buffer size is wrong, but its known it only needs
       32 bytes to decode the keys*/
    size_t len = C25519_KEY_SIZE + 3;
    base64_decode(b64_pk, strlen(b64_pk), desire_keys.pk, &len);
    TEST_ASSERT_EQUAL_INT(C25519_KEY_SIZE, len);
    len = C25519_KEY_SIZE + 3;
    base64_decode(b64_sk, strlen(b64_sk), desire_keys.sk, &len);
    TEST_ASSERT_EQUAL_INT(C25519_KEY_SIZE, len);
    len = C25519_KEY_SIZE + 3;
    base64_decode(b64_ebid, strlen(b64_ebid), ebid, &len);
    TEST_ASSERT_EQUAL_INT(C25519_KEY_SIZE, len);
    /* the estimated buffer size is wrong, but its known it only needs
       32 bytes to decode the pets */
    len = C25519_KEY_SIZE + 3;
    base64_decode(b64_expected_pet_1, strlen(b64_expected_pet_1), expected_pet_1, &len);
    TEST_ASSERT_EQUAL_INT(PET_SIZE, len);
    len = C25519_KEY_SIZE + 3;
    base64_decode(b64_expected_pet_2, strlen(b64_expected_pet_2), expected_pet_2, &len);
    TEST_ASSERT_EQUAL_INT(PET_SIZE, len);
    /* the estimated buffer size is wrong, but its known it only needs
       32 bytes to decode the shared secret */
    len = C25519_KEY_SIZE + 3;
    base64_decode(b64_secret, strlen(b64_secret), expected_secret, &len);
    TEST_ASSERT_EQUAL_INT(PET_SIZE, len);
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
    uint8_t secret[C25519_KEY_SIZE] = {0};
    crypto_manager_shared_secret(desire_keys.sk, ebid, secret);
    TEST_ASSERT(memcmp(secret, expected_secret, C25519_KEY_SIZE) == 0);
}

static void test_crypto_manager_gen_pet(void)
{
    uint8_t pet_1[PET_SIZE] = {0};
    uint8_t pet_2[PET_SIZE] = {0};
    TEST_ASSERT(crypto_manager_gen_pet(&desire_keys, ebid, 0x01, pet_1) == 0);
    TEST_ASSERT(crypto_manager_gen_pet(&desire_keys, ebid, 0x02, pet_2) == 0);
    TEST_ASSERT(memcmp(pet_1, expected_pet_1, PET_SIZE) == 0);
    TEST_ASSERT(memcmp(pet_2, expected_pet_2, PET_SIZE) == 0);
}

Test *tests_crypto_manager(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_crypto_manager_gen_keypair),
        new_TestFixture(test_crypto_manager_shared_secret),
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
