
#include <string.h>

#include "embUnit.h"
#include "ebid.h"
#include "random.h"
#include "crypto_manager.h"

static const uint8_t desire_ebid_slice_1[EBID_SLICE_SIZE_LONG] = {
    0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x20, 0x2e
};

static const uint8_t desire_ebid_slice_2[EBID_SLICE_SIZE_LONG] = {
    0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x20, 0xfe
};

static const uint8_t desire_ebid_slice_3[EBID_SLICE_SIZE_SHORT] = {
    0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17
};

static const uint8_t desire_ebid[EBID_SIZE] = {
    0x20, 0x21, 0x22, 0x23, 0x24,
    0x25, 0x26, 0x27, 0x28, 0x29,
    0x20, 0x2e, 0x10, 0x11, 0x12,
    0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x20, 0xfe, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15,
    0x16, 0x17
};

static const uint8_t desire_ebid_xor[EBID_SLICE_SIZE_LONG] = {
    0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27,
    0x30, 0x30, 0x00, 0xd0
};

static crypto_manager_keys_t keys = {
    .pk = {
        0x5c, 0x24, 0x4c, 0x6e, 0xf9, 0x7a, 0x02, 0x9c,
        0x83, 0xe3, 0x67, 0xac, 0x3c, 0x31, 0xd0, 0x20,
        0x97, 0xdc, 0x59, 0xf8, 0xab, 0xe4, 0xa5, 0xb8,
        0xf6, 0xd9, 0x07, 0x11, 0x3d, 0xce, 0x90, 0x25
    },
    .sk = {
        0x7a, 0xdd, 0xb8, 0x16, 0x6f, 0x48, 0x01, 0xb7,
        0x25, 0xa5, 0x0f, 0x4b, 0x69, 0x64, 0x93, 0xf2,
        0xf4, 0x12, 0x4a, 0x3e, 0x72, 0x6b, 0x10, 0xda,
        0x45, 0x83, 0x18, 0xe6, 0xff, 0x12, 0xaf, 0x50
    }
};

static const uint8_t ebid_slice_1[EBID_SLICE_SIZE_LONG] = {
    0x5c, 0x24, 0x4c, 0x6e,
    0xf9, 0x7a, 0x02, 0x9c,
    0x83, 0xe3, 0x67, 0xac,
};

static const uint8_t ebid_slice_2[EBID_SLICE_SIZE_LONG] = {
    0x3c, 0x31, 0xd0, 0x20,
    0x97, 0xdc, 0x59, 0xf8,
    0xab, 0xe4, 0xa5, 0xb8,
};

static const uint8_t ebid_slice_3[EBID_SLICE_SIZE_SHORT] = {
    0xf6, 0xd9, 0x07, 0x11,
    0x3d, 0xce, 0x90, 0x25
};

static const uint8_t ebid_xor[EBID_SLICE_SIZE_LONG] = {
    0x96, 0xcc, 0x9b, 0x5f,
    0x53, 0x68, 0xcb, 0x41,
    0x28, 0x07, 0xc2, 0x14
};

static ebid_t ebid;

static void setUp(void)
{
    random_init(0);
    ebid_init(&ebid);
}

static void tearDown(void)
{
    /* Finalize */
}

static void test_ebid_generate(void)
{
    ebid_generate(&ebid, &keys);
    TEST_ASSERT(memcmp(keys.pk, ebid.parts.ebid.u8, sizeof(EBID_SIZE)) == 0);
    TEST_ASSERT(memcmp(ebid_get_slice1(&ebid), ebid_slice_1, sizeof(EBID_SLICE_SIZE_LONG)) == 0);
    TEST_ASSERT(memcmp(ebid_get_slice2(&ebid), ebid_slice_2, sizeof(EBID_SLICE_SIZE_LONG)) == 0);
    TEST_ASSERT(memcmp(ebid_get_slice3(&ebid), ebid_slice_3, sizeof(EBID_SLICE_SIZE_SHORT)) == 0);
    uint8_t zero_buffer[4] = {0};
    TEST_ASSERT(memcmp(&ebid.parts.ebid.slice.ebid_3[EBID_SLICE_SIZE_SHORT], zero_buffer, sizeof(zero_buffer)) == 0);
    TEST_ASSERT(memcmp(ebid_get_xor(&ebid), ebid_xor, sizeof(EBID_SLICE_SIZE_LONG)) == 0);
}

static void test_ebid_reconstruct_FAIL(void)
{
    ebid_set_slice2(&ebid, ebid_slice_2);
    ebid_set_slice3(&ebid, ebid_slice_3);
    TEST_ASSERT(ebid_reconstruct(&ebid));
}

static void test_ebid_reconstruct_SUCCESS_ALL_SET(void)
{
    ebid_generate(&ebid, &keys);
    TEST_ASSERT(ebid_reconstruct(&ebid) == 0);
}

static void test_ebid_reconstruct_SUCCESS_1(void)
{
    ebid_set_slice2(&ebid, ebid_slice_2);
    ebid_set_slice3(&ebid, ebid_slice_3);
    ebid_set_xor(&ebid, ebid_xor);
    ebid_reconstruct(&ebid);
    TEST_ASSERT(memcmp(ebid_get_slice1(&ebid), ebid_slice_1, sizeof(EBID_SLICE_SIZE_LONG)) == 0);
}

static void test_ebid_reconstruct_SUCCESS_2(void)
{
    ebid_set_slice1(&ebid, ebid_slice_1);
    ebid_set_slice3(&ebid, ebid_slice_3);
    ebid_set_xor(&ebid, ebid_xor);
    ebid_reconstruct(&ebid);
    TEST_ASSERT(memcmp(ebid_get_slice2(&ebid), ebid_slice_2, sizeof(EBID_SLICE_SIZE_LONG)) == 0);
}

static void test_ebid_reconstruct_SUCCESS_3(void)
{
    ebid_set_slice1(&ebid, ebid_slice_1);
    ebid_set_slice2(&ebid, ebid_slice_2);
    ebid_set_xor(&ebid, ebid_xor);
    ebid_reconstruct(&ebid);
    TEST_ASSERT(memcmp(ebid_get_slice3(&ebid), ebid_slice_3, sizeof(EBID_SLICE_SIZE_LONG)) == 0);
}

static void test_ebid_reconstruct_SUCCESS_xor(void)
{
    ebid_set_slice1(&ebid, ebid_slice_1);
    ebid_set_slice2(&ebid, ebid_slice_2);
    ebid_set_slice3(&ebid, ebid_slice_3);
    ebid_reconstruct(&ebid);
    TEST_ASSERT(memcmp(ebid_get_xor(&ebid), ebid_xor, sizeof(EBID_SLICE_SIZE_LONG)) == 0);
}

static void test_ebid_generate_desire(void)
{
    /* set the desire ebid as pk */
    memcpy(keys.pk, desire_ebid, EBID_SIZE);
    ebid_generate(&ebid, &keys);
    TEST_ASSERT(memcmp(desire_ebid, ebid.parts.ebid.u8, sizeof(EBID_SIZE)) == 0);
    TEST_ASSERT(memcmp(ebid_get_slice1(&ebid), desire_ebid_slice_1, sizeof(EBID_SLICE_SIZE_LONG)) == 0);
    TEST_ASSERT(memcmp(ebid_get_slice2(&ebid), desire_ebid_slice_2, sizeof(EBID_SLICE_SIZE_LONG)) == 0);
    TEST_ASSERT(memcmp(ebid_get_slice3(&ebid), desire_ebid_slice_3, sizeof(EBID_SLICE_SIZE_SHORT)) == 0);
}

static void test_ebid_reconstruct_desire(void)
{
    /* set the desire ebid as pk */
    ebid_set_slice1(&ebid, desire_ebid_slice_1);
    ebid_set_slice2(&ebid, desire_ebid_slice_2);
    ebid_set_slice3(&ebid, desire_ebid_slice_3);
    ebid_reconstruct(&ebid);
    TEST_ASSERT(memcmp(desire_ebid, ebid.parts.ebid.u8, sizeof(EBID_SIZE)) == 0);
}

static void test_ebid_reconstruct_FAIL_desire(void)
{
    /* set the desire ebid as pk */
    ebid_set_slice1(&ebid, desire_ebid_slice_1);
    ebid_set_slice2(&ebid, desire_ebid_slice_2);
    TEST_ASSERT(ebid_reconstruct(&ebid));
}

static void test_ebid_reconstruct_SUCCESS_1_desire(void)
{
    /* set the desire ebid as pk */
    ebid_set_slice1(&ebid, desire_ebid_slice_1);
    ebid_set_slice2(&ebid, desire_ebid_slice_2);
    ebid_set_xor(&ebid, desire_ebid_xor);
    ebid_reconstruct(&ebid);
    TEST_ASSERT(memcmp(desire_ebid, ebid.parts.ebid.u8, sizeof(EBID_SIZE)) == 0);
}

Test *tests_ebid_all(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_ebid_generate),
        new_TestFixture(test_ebid_reconstruct_SUCCESS_ALL_SET),
        new_TestFixture(test_ebid_reconstruct_FAIL),
        new_TestFixture(test_ebid_reconstruct_SUCCESS_1),
        new_TestFixture(test_ebid_reconstruct_SUCCESS_2),
        new_TestFixture(test_ebid_reconstruct_SUCCESS_3),
        new_TestFixture(test_ebid_reconstruct_SUCCESS_xor),
        /* desire test set */
        new_TestFixture(test_ebid_generate_desire),
        new_TestFixture(test_ebid_reconstruct_desire),
        new_TestFixture(test_ebid_reconstruct_FAIL_desire),
        new_TestFixture(test_ebid_reconstruct_SUCCESS_1_desire),
    };

    EMB_UNIT_TESTCALLER(ebid_tests, setUp, tearDown, fixtures);
    return (Test*)&ebid_tests;
}

void tests_ebid(void)
{
    TESTS_RUN(tests_ebid_all());
}
