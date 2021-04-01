
#include <string.h>
#include <stdio.h>

#include "base64.h"
#include "embUnit.h"
#include "ebid.h"
#include "random.h"
#include "crypto_manager.h"

static crypto_manager_keys_t keys_bob = {
    .pk = {
        0xbf, 0x0a, 0x8c, 0x1e, 0x3a, 0xe9, 0x62, 0xbb,
        0xb7, 0xb3, 0x70, 0x61, 0x64, 0x9a, 0x8d, 0xa5,
        0xdb, 0xfb, 0xc9, 0x54, 0xdc, 0xba, 0x4b, 0xfd,
        0x8f, 0x6d, 0x8f, 0x34, 0x71, 0x33, 0x4a, 0x42
    },
    .sk = {
        0x50, 0x51, 0x93, 0x40, 0x2b, 0x31, 0xbb, 0x77,
        0xfb, 0x97, 0x64, 0x2c, 0x2b, 0x0a, 0x67, 0x8a,
        0x64, 0x96, 0xd6, 0xf7, 0xee, 0x04, 0x1a, 0x77,
        0x0b, 0x60, 0xbc, 0xad, 0xd0, 0x26, 0x83, 0x5e
    }
};

static crypto_manager_keys_t keys_alice = {
    .pk = {
        0x10, 0x37, 0xc5, 0xc7, 0xec, 0x40, 0x5e, 0xbb,
        0x00, 0x68, 0x82, 0x5a, 0x35, 0xb5, 0x20, 0x75,
        0x51, 0x5f, 0xd1, 0x64, 0xe4, 0xb5, 0x92, 0x22,
        0xc8, 0x9c, 0x33, 0x84, 0x5e, 0xdd, 0xa8, 0x14,
    },
    .sk = {
        0xd8, 0x80, 0xc6, 0x76, 0x69, 0xcb, 0x97, 0x62,
        0x43, 0x05, 0x1c, 0x5f, 0x8d, 0x5b, 0x02, 0xe4,
        0xc3, 0x2a, 0x31, 0xd0, 0x35, 0x94, 0x68, 0xe5,
        0xab, 0x35, 0x23, 0x9e, 0x59, 0x92, 0xf4, 0x4c
    }
};

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
}

static void tearDown(void)
{
    /* Finalize */
}

static void test_crypto_manager_gen_pets(void)
{
    ebid_t bob;
    ebid_t alice;
    pet_t pet_bob;
    pet_t pet_alice;

    /* init and generate the ebid */
    ebid_init(&bob);
    ebid_init(&alice);
    ebid_generate(&bob, &keys_bob);
    ebid_generate(&alice, &keys_alice);
    /* generate pets */
    crypto_manager_gen_pets(&keys_alice, bob.parts.ebid.u8, &pet_alice);
    crypto_manager_gen_pets(&keys_bob, alice.parts.ebid.u8, &pet_bob);
    /* validate pets */
    TEST_ASSERT(memcmp(pet_bob.et, pet_alice.rt, sizeof(pet_bob.et)) == 0);
    TEST_ASSERT(memcmp(pet_alice.et, pet_bob.rt, sizeof(pet_bob.et)) == 0);
}

static void test_crypto_manager_gen_pets_desire(void)
{
    int ret;
    ebid_t bob;
    ebid_t alice;
    pet_t pet_bob;
    pet_t pet_alice;

    /* decode desire test vectors */
    _b64_decode_test_vectors();
    /* init and generate the ebid*/
    ebid_init(&bob);
    ebid_init(&alice);
    ret = ebid_generate(&bob, &desire_keys_bob);
    TEST_ASSERT_EQUAL_INT(ret, 0);
    ret = ebid_generate(&alice, &desire_keys_alice);
    TEST_ASSERT_EQUAL_INT(ret, 0);
    /* generate pets */
    ret = crypto_manager_gen_pets(&desire_keys_alice, bob.parts.ebid.u8,
                                  &pet_alice);
    TEST_ASSERT_EQUAL_INT(ret, 0);
    ret = crypto_manager_gen_pets(&desire_keys_bob, alice.parts.ebid.u8,
                                  &pet_bob);
    TEST_ASSERT_EQUAL_INT(ret, 0);
    /* validate pets */
    TEST_ASSERT(memcmp(pet_bob.et, expected_pet_1, PET_SIZE) == 0);
    TEST_ASSERT(memcmp(pet_bob.rt, expected_pet_2, PET_SIZE) == 0);
    TEST_ASSERT(memcmp(pet_alice.rt, expected_pet_1, PET_SIZE) == 0);
    TEST_ASSERT(memcmp(pet_alice.et, expected_pet_2, PET_SIZE) == 0);
    TEST_ASSERT(memcmp(pet_bob.et, pet_alice.rt, sizeof(pet_bob.et)) == 0);
    TEST_ASSERT(memcmp(pet_alice.et, pet_bob.rt, sizeof(pet_bob.et)) == 0);
}

Test *tests_ebid(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_crypto_manager_gen_pets),
        new_TestFixture(test_crypto_manager_gen_pets_desire),
    };

    EMB_UNIT_TESTCALLER(ebid_tests, setUp, tearDown, fixtures);
    return (Test *)&ebid_tests;
}

int main(void)
{
    TESTS_START();
    TESTS_RUN(tests_ebid());
    TESTS_END();

    return 0;
}
