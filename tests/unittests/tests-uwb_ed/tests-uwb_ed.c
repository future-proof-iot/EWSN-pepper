#include <string.h>
#include <math.h>

#include "embUnit.h"
#include "uwb_ed.h"
#include "clist.h"

#include "bpf/uwb_ed_shared.h"

static const uint8_t ebid_slice[][EBID_SLICE_SIZE_LONG] = {
    { 0x5c, 0x24, 0x4c, 0x6e, 0xf9, 0x7a, 0x02, 0x9c, 0x83, 0xe3, 0x67, 0xac },
    { 0x3c, 0x31, 0xd0, 0x20, 0x97, 0xdc, 0x59, 0xf8, 0xab, 0xe4, 0xa5, 0xb8 },
    { 0xf6, 0xd9, 0x07, 0x11, 0x3d, 0xce, 0x90, 0x25, 0x00, 0x00, 0x00, 0x00 },
    { 0x96, 0xcc, 0x9b, 0x5f, 0x53, 0x68, 0xcb, 0x41, 0x28, 0x07, 0xc2, 0x14 },
};

static const uint8_t ebid[EBID_SIZE] = {
    0x5c, 0x24, 0x4c, 0x6e, 0xf9, 0x7a, 0x02, 0x9c,
    0x83, 0xe3, 0x67, 0xac, 0x3c, 0x31, 0xd0, 0x20,
    0x97, 0xdc, 0x59, 0xf8, 0xab, 0xe4, 0xa5, 0xb8,
    0xf6, 0xd9, 0x07, 0x11, 0x3d, 0xce, 0x90, 0x25,
};

static const uint8_t local_ebid_1[EBID_SIZE] = {
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x20, 0x2e, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20, 0xfe,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
};

static ebid_t local_ebid;
static uwb_ed_list_t list;
static uwb_ed_memory_manager_t manager;

static void setUp(void)
{
    ebid_init(&local_ebid);
    memcpy(local_ebid.parts.ebid.u8, local_ebid_1, EBID_SIZE);
    local_ebid.status.status = EBID_HAS_ALL;
    uwb_ed_memory_manager_init(&manager);
    uwb_ed_list_init(&list, &manager, &local_ebid);
}

static void tearDown(void)
{
    /* Finalize */
}

static void test_uwb_ed_init(void)
{
    uwb_ed_t uwb_ed;

    uwb_ed_init(&uwb_ed, 0x01);
    TEST_ASSERT(uwb_ed.cid == 0x01);
}

static void test_uwb_ed_add_remove(void)
{
    uwb_ed_t uwb_ed_1;
    uwb_ed_t uwb_ed_2;

    uwb_ed_init(&uwb_ed_1, 0x01);
    uwb_ed_init(&uwb_ed_2, 0x02);
    uwb_ed_add(&list, &uwb_ed_1);
    uwb_ed_t *first = (uwb_ed_t *)clist_rpeek(&list.list);
    TEST_ASSERT(memcmp(first, &uwb_ed_1, sizeof(uwb_ed_t)) == 0);
    uwb_ed_add(&list, &uwb_ed_2);
    first = (uwb_ed_t *)clist_rpeek(&list.list);
    TEST_ASSERT(memcmp(first, &uwb_ed_2, sizeof(uwb_ed_t)) == 0);
    uwb_ed_remove(&list, &uwb_ed_2);
    first = (uwb_ed_t *)clist_rpeek(&list.list);
    TEST_ASSERT(memcmp(first, &uwb_ed_1, sizeof(uwb_ed_t)) == 0);
    uwb_ed_remove(&list, &uwb_ed_1);
    TEST_ASSERT(clist_count(&list.list) == 0);
}

static void test_uwb_ed_list_get_by_cid(void)
{
    uwb_ed_t *none = uwb_ed_list_get_by_cid(&list, 0);

    TEST_ASSERT(none == NULL);
    uwb_ed_t uwb_ed_1;
    uwb_ed_t uwb_ed_2;
    uwb_ed_t uwb_ed_3;
    uwb_ed_init(&uwb_ed_1, 0x01);
    uwb_ed_init(&uwb_ed_2, 0x02);
    uwb_ed_init(&uwb_ed_3, 0x03);
    uwb_ed_add(&list, &uwb_ed_1);
    uwb_ed_add(&list, &uwb_ed_2);
    uwb_ed_add(&list, &uwb_ed_3);
    TEST_ASSERT(clist_count(&list.list) == 3);
    uwb_ed_t *first = uwb_ed_list_get_by_cid(&list, 0x01);
    uwb_ed_t *second = uwb_ed_list_get_by_cid(&list, 0x02);
    uwb_ed_t *third = uwb_ed_list_get_by_cid(&list, 0x03);
    TEST_ASSERT(memcmp(first, &uwb_ed_1, sizeof(uwb_ed_t)) == 0);
    TEST_ASSERT(memcmp(second, &uwb_ed_2, sizeof(uwb_ed_t)) == 0);
    TEST_ASSERT(memcmp(third, &uwb_ed_3, sizeof(uwb_ed_t)) == 0);
}

static void test_uwb_ed_list_get_by_short_addr(void)
{
    uwb_ed_t *none = uwb_ed_list_get_by_short_addr(&list, 0);

    TEST_ASSERT(none == NULL);
    uwb_ed_t uwb_ed_1;
    uwb_ed_t uwb_ed_2;
    uwb_ed_init(&uwb_ed_1, 0x12345678);
    uwb_ed_init(&uwb_ed_2, 0x56781234);
    uwb_ed_add(&list, &uwb_ed_1);
    uwb_ed_add(&list, &uwb_ed_2);
    TEST_ASSERT(clist_count(&list.list) == 2);
    uwb_ed_t *first =
        uwb_ed_list_get_by_short_addr(&list, uwb_get_short_addr(&uwb_ed_1));
    uwb_ed_t *second =
        uwb_ed_list_get_by_short_addr(&list, uwb_get_short_addr(&uwb_ed_2));
    TEST_ASSERT(memcmp(first, &uwb_ed_1, sizeof(uwb_ed_t)) == 0);
    TEST_ASSERT(memcmp(second, &uwb_ed_2, sizeof(uwb_ed_t)) == 0);
}

static void test_uwb_ed_exposure_time(void)
{
    uwb_ed_t uwb_ed;

    uwb_ed_init(&uwb_ed, 0x01);
    uwb_ed.seen_first_s = 10;
    uwb_ed.seen_last_s = 20;
    TEST_ASSERT_EQUAL_INT(10, uwb_ed_exposure_time(&uwb_ed));
}

static void test_uwb_ed_add_slice(void)
{
    uwb_ed_t uwb_ed;

    uwb_ed_init(&uwb_ed, 0x01);
    uwb_ed_add_slice(&uwb_ed, 0, ebid_slice[0], EBID_SLICE_1);
    uwb_ed_add_slice(&uwb_ed, 1, ebid_slice[1], EBID_SLICE_2);
    uint8_t pad_slice3[EBID_SLICE_SIZE_LONG];
    memset(pad_slice3, '\0', EBID_SLICE_SIZE_LONG);
    memcpy(pad_slice3 + EBID_SLICE_SIZE_PAD, ebid_slice[2],
           EBID_SLICE_SIZE_SHORT);
    uwb_ed_add_slice(&uwb_ed, 3, pad_slice3, EBID_SLICE_3);
    TEST_ASSERT(memcmp(ebid, uwb_ed.ebid.parts.ebid.u8, EBID_SIZE) == 0);
}

static void test_uwb_ed_process_data(void)
{
    uwb_ed_t uwb_ed;

    uwb_ed_init(&uwb_ed, 0x01);
    uwb_ed_process_data(&uwb_ed, 10, 200);
    uwb_ed_process_data(&uwb_ed, 40, 100);
    uwb_ed_process_data(&uwb_ed, 300, 50);
    uwb_ed_process_data(&uwb_ed, MIN_EXPOSURE_TIME_S, 50);
    TEST_ASSERT_EQUAL_INT(400, uwb_ed.cumulative_d_cm);
    TEST_ASSERT_EQUAL_INT(MIN_EXPOSURE_TIME_S, uwb_ed.seen_last_s);
    TEST_ASSERT_EQUAL_INT(4, uwb_ed.req_count);
    TEST_ASSERT_EQUAL_INT(true, uwb_ed_finish(&uwb_ed));
    TEST_ASSERT_EQUAL_INT(100, uwb_ed.cumulative_d_cm);
}

static void test_uwb_ed_list_process(void)
{
    uint32_t cid_1 = 1;
    uint32_t cid_2 = 2;
    uint32_t cid_3 = 3;

    /* new encounter that will have the EBID reconstructed */
    TEST_ASSERT(clist_count(&list.list) == 0);
    uwb_ed_list_process_slice(&list, cid_1, 0, ebid_slice[2], EBID_SLICE_3);
    TEST_ASSERT(clist_count(&list.list) == 1);
    uwb_ed_list_process_slice(&list, cid_1, 0, ebid_slice[1], EBID_SLICE_2);
    uwb_ed_list_process_slice(&list, cid_1, 0, ebid_slice[0], EBID_SLICE_1);
    /* add data to the new encounter, ebid should be reconstructed, data is
       added with enough exposure time */
    uwb_ed_list_process_rng_data(&list, (uint16_t)cid_1, MIN_EXPOSURE_TIME_S,
                                 200);
    uwb_ed_list_process_rng_data(&list, (uint16_t)cid_1, MIN_EXPOSURE_TIME_S,
                                 100);
    uwb_ed_list_process_rng_data(&list, (uint16_t)cid_1, MIN_EXPOSURE_TIME_S,
                                 150);

    /* new encounter that will have EBID reconstructed */
    uwb_ed_list_process_slice(&list, cid_2, 0, ebid_slice[2], EBID_SLICE_3);
    TEST_ASSERT(clist_count(&list.list) == 2);
    uwb_ed_list_process_slice(&list, cid_2, 0, ebid_slice[1], EBID_SLICE_2);
    uwb_ed_list_process_slice(&list, cid_2, 0, ebid_slice[0], EBID_SLICE_1);
    /* data is added but with not enough exposure time */
    uwb_ed_list_process_rng_data(&list, (uint16_t)cid_2,
                                 MIN_EXPOSURE_TIME_S - 1, 150);
    uwb_ed_list_process_rng_data(&list, (uint16_t)cid_2,
                                 MIN_EXPOSURE_TIME_S - 1, 100);

    /* new encounter that wont be able to reconstruct the EBID */
    uwb_ed_list_process_slice(&list, cid_3, 0, ebid_slice[2], EBID_SLICE_3);
    TEST_ASSERT(clist_count(&list.list) == 3);

    /* finish list processing, only one encounter should remain */
    uwb_ed_list_finish(&list);
    TEST_ASSERT_EQUAL_INT(1, clist_count(&list.list));
}

#if IS_USED(MODULE_BPF)
static void test_uwb_ed_finish_rbpf(void)
{
    uwb_ed_t ed;
    ed.cumulative_d_cm = MAX_DISTANCE_CM * 4;
    ed.seen_first_s = 0;
    ed.seen_last_s = MIN_EXPOSURE_TIME_S + 1;
    ed.ebid.status.status = EBID_HAS_ALL;
    ed.req_count = 4;
    TEST_ASSERT_EQUAL_INT(true, uwb_ed_finish_bpf(&ed));
    TEST_ASSERT_EQUAL_INT(MAX_DISTANCE_CM, ed.cumulative_d_cm);
    ed.cumulative_d_cm = MAX_DISTANCE_CM * 4;
    ed.req_count = 2;
    TEST_ASSERT_EQUAL_INT(false, uwb_ed_finish_bpf(&ed));
    ed.cumulative_d_cm = MAX_DISTANCE_CM * 4;
    ed.req_count = 4;
    ed.seen_last_s = 1;
    TEST_ASSERT_EQUAL_INT(false, uwb_ed_finish_bpf(&ed));
}
#endif

Test *tests_uwb_ed_all(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_uwb_ed_init),
        new_TestFixture(test_uwb_ed_add_remove),
        new_TestFixture(test_uwb_ed_list_get_by_cid),
        new_TestFixture(test_uwb_ed_list_get_by_short_addr),
        new_TestFixture(test_uwb_ed_exposure_time),
        new_TestFixture(test_uwb_ed_add_slice),
        new_TestFixture(test_uwb_ed_process_data),
        new_TestFixture(test_uwb_ed_list_process),
#if IS_USED(MODULE_BPF)
        new_TestFixture(test_uwb_ed_finish_rbpf),
#endif
    };

    EMB_UNIT_TESTCALLER(uwb_ed_tests, setUp, tearDown, fixtures);
    return (Test *)&uwb_ed_tests;
}

void tests_uwb_ed(void)
{
    TESTS_RUN(tests_uwb_ed_all());
}
