
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

static crypto_manager_keys_t keys_bob_desire;
static crypto_manager_keys_t keys_alice_desire;
static uint8_t pet_1[32];
static uint8_t pet_2[32];
static const char b64_bob_pk[]   = "wsPGv9bw2Q0mLNR8Q+TA5q4e6GQzRIiPxl5gXtrsZi8=";
static const char b64_bob_sk[]   = "IAB2ptLioKUQA+T4SH8AAAAAAAAAAAAAEAPk+Eh/AEA=";
static const char b64_alice_pk[] = "l9OSbVr/VD2XhyCbVdvaRoIjhWxCuW1iWaQ0GeHkLzo=";
static const char b64_alice_sk[] = "OBgpK9a1/XWQruL4SH8AAAAAAAAAAAAAAMfgNUl/AEA=";
static const char b64_pet_1[]    = "KqO9fF5bvHtJFh6uWSDBnaO4JZu6hi/AJTjLbSyPklE=";
static const char b64_pet_2[]    = "iscm1Ih0+xfKL38bF/jONgeGkhqSKaWyaokgxGiT+1U=";

static void setUp(void)
{
    random_init(1);
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
    ebid_init(&bob);
    ebid_init(&alice);
    ebid_generate(&bob, &keys_bob);
    ebid_generate(&alice, &keys_alice);
    crypto_manager_gen_pets(&keys_alice, bob.parts.ebid.u8, &pet_alice);
    crypto_manager_gen_pets(&keys_bob, alice.parts.ebid.u8, &pet_bob);
    TEST_ASSERT(memcmp(pet_bob.et, pet_alice.rt, sizeof(pet_bob.et)) == 0);
    TEST_ASSERT(memcmp(pet_alice.et, pet_bob.rt, sizeof(pet_bob.et)) == 0);
}

static void _desire_b64_decode(void)
{
    /* the estimated buffer size is wrong, but its known it only needs
       32 bytes to decode the keys*/
    size_t len = C25519_KEY_SIZE + 3;
    base64_decode(b64_bob_sk, strlen(b64_bob_sk), keys_bob_desire.sk, &len);
    TEST_ASSERT_EQUAL_INT(C25519_KEY_SIZE, len);
    len = C25519_KEY_SIZE + 3;
    base64_decode(b64_bob_pk, strlen(b64_bob_pk), keys_bob_desire.pk, &len);
    TEST_ASSERT_EQUAL_INT(C25519_KEY_SIZE, len);
    len = C25519_KEY_SIZE + 3;
    base64_decode(b64_alice_pk, strlen(b64_alice_pk), keys_alice_desire.pk, &len);
    TEST_ASSERT_EQUAL_INT(C25519_KEY_SIZE, len);
    len = C25519_KEY_SIZE + 3;
    base64_decode(b64_alice_sk, strlen(b64_alice_sk), keys_alice_desire.sk, &len);
    TEST_ASSERT_EQUAL_INT(C25519_KEY_SIZE, len);
    len = C25519_KEY_SIZE + 3;
    /* the estimated buffer size is wrong, but its known it only needs
       32 bytes to decode the pets */
    base64_decode(b64_pet_1, strlen(b64_pet_1), pet_1, &len);
    TEST_ASSERT_EQUAL_INT(PET_SIZE, len);
    len = C25519_KEY_SIZE + 3;
    base64_decode(b64_pet_2, strlen(b64_pet_2), pet_2, &len);
    TEST_ASSERT_EQUAL_INT(PET_SIZE, len);
}

static void test_crypto_manager_gen_pets_desire(void)
{
    _desire_b64_decode();
    ebid_t bob;
    ebid_t alice;
    pet_t pet_bob;
    pet_t pet_alice;
    ebid_init(&bob);
    ebid_init(&alice);
    ebid_generate(&bob, &keys_bob_desire);
    ebid_generate(&alice, &keys_alice_desire);
    crypto_manager_gen_pets(&keys_alice_desire, bob.parts.ebid.u8, &pet_alice);
    crypto_manager_gen_pets(&keys_bob_desire, alice.parts.ebid.u8, &pet_bob);
    TEST_ASSERT(memcmp(pet_bob.et, pet_1, sizeof(pet_bob.et)) == 0);
    TEST_ASSERT(memcmp(pet_bob.rt, pet_2, sizeof(pet_bob.rt)) == 0);
}

Test *tests_ebid(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_crypto_manager_gen_pets),
        new_TestFixture(test_crypto_manager_gen_pets_desire),
    };

    EMB_UNIT_TESTCALLER(ebid_tests, setUp, tearDown, fixtures);
    return (Test*)&ebid_tests;
}

int main(void)
{
    TESTS_START();
    TESTS_RUN(tests_ebid());
    TESTS_END();

    return 0;
}
