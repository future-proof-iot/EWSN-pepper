#include <string.h>

#include "embUnit.h"
#include "edl.h"
#include "clist.h"

static edl_list_t list;

#define TEST_EDL_RSSI_1     (-80)
#define TEST_EDL_RSSI_2     (-70)
#define TEST_EDL_RSSI_3     (-60)

#define TEST_EDL_RANGE_1     (2.2)
#define TEST_EDL_RANGE_2     (3.3)
#define TEST_EDL_RANGE_3     (4.4)


static const uint8_t ebid[] = {
    0x20, 0x21, 0x22, 0x23, 0x24,
    0x25, 0x26, 0x27, 0x28, 0x29,
    0x20, 0x2e, 0x10, 0x11, 0x12,
    0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x20, 0xfe, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15,
    0x16, 0x17
};

static void setUp(void)
{
    edl_list_init(&list, (void*) ebid);
}

static void tearDown(void)
{
    /* Finalize */
}

static void test_edl_add(void)
{
    edl_t edl_1;
    edl_t edl_2;
    edl_init_rssi(&edl_1, TEST_EDL_RSSI_1, 0);
    edl_init_range(&edl_2, TEST_EDL_RANGE_1, 1);
    edl_add(&list, &edl_1);
    edl_add(&list, &edl_2);
    edl_t *first = (edl_t*) clist_lpeek(&list.list);
    edl_t *last = (edl_t*) clist_rpeek(&list.list);
    TEST_ASSERT(memcmp(first, &edl_1, sizeof(edl_t)) == 0);
    TEST_ASSERT(memcmp(last, &edl_2, sizeof(edl_t)) == 0);
}

static void test_edl_remove(void)
{
    edl_t edl;
    edl_init_rssi(&edl, TEST_EDL_RSSI_1, 0);
    edl_add(&list, &edl);
    edl_t *first = (edl_t*) clist_lpeek(&list.list);
    TEST_ASSERT(memcmp(first, &edl, sizeof(edl_t)) == 0);
    edl_remove(&list, &edl);
    TEST_ASSERT(clist_count(&list.list) == 0);
}

static void test_edl_list_get_nth(void)
{
    edl_t *none = edl_list_get_nth(&list, 0);
    TEST_ASSERT(none == NULL);
    edl_t edl_1;
    edl_t edl_2;
    edl_t edl_3;
    edl_init_rssi(&edl_1, TEST_EDL_RSSI_1, 0);
    edl_init_rssi(&edl_2, TEST_EDL_RSSI_2, 1);
    edl_init_rssi(&edl_3, TEST_EDL_RSSI_3, 3);
    edl_add(&list, &edl_1);
    edl_add(&list, &edl_2);
    edl_add(&list, &edl_3);
    edl_t *first = edl_list_get_nth(&list, 0);
    edl_t *second = edl_list_get_nth(&list, 1);
    edl_t *third = edl_list_get_nth(&list, 2);
    TEST_ASSERT(memcmp(first, &edl_1, sizeof(edl_t)) == 0);
    TEST_ASSERT(memcmp(second, &edl_2, sizeof(edl_t)) == 0);
    TEST_ASSERT(memcmp(third, &edl_3, sizeof(edl_t)) == 0);
}

static void test_edl_list_get_before(void)
{
    edl_t edl_1;
    edl_t edl_2;
    edl_t edl_3;
    edl_init_rssi(&edl_1, TEST_EDL_RSSI_1, 2);
    edl_init_rssi(&edl_2, TEST_EDL_RSSI_2, 1);
    edl_init_rssi(&edl_3, TEST_EDL_RSSI_3, 1);
    edl_add(&list, &edl_1);
    edl_add(&list, &edl_2);
    edl_add(&list, &edl_3);
    edl_t *edl = edl_list_get_before(&list, 2);
    TEST_ASSERT(memcmp(edl, &edl_2, sizeof(edl_t)) == 0);
    edl = edl_list_get_before(&list, 0);
    TEST_ASSERT(edl == NULL);
}

static void test_edl_list_get_after(void)
{
    edl_t edl_1;
    edl_t edl_2;
    edl_t edl_3;
    edl_init_rssi(&edl_1, TEST_EDL_RSSI_1, 1);
    edl_init_rssi(&edl_2, TEST_EDL_RSSI_2, 3);
    edl_init_rssi(&edl_3, TEST_EDL_RSSI_3, 3);
    edl_add(&list, &edl_1);
    edl_add(&list, &edl_2);
    edl_add(&list, &edl_3);
    edl_t *edl = edl_list_get_after(&list, 1);
    TEST_ASSERT(memcmp(edl, &edl_2, sizeof(edl_t)) == 0);
    edl = edl_list_get_after(&list, 3);
    TEST_ASSERT(edl == NULL);
}

static void test_edl_list_get_by_time(void)
{
    edl_t edl_1;
    edl_init_rssi(&edl_1, TEST_EDL_RSSI_1, 1);
    edl_add(&list, &edl_1);
    edl_t *edl = edl_list_get_by_time(&list, 2);
    TEST_ASSERT(edl == NULL);
    edl_t edl_2;
    edl_init_rssi(&edl_2, TEST_EDL_RSSI_2, 2);
    edl_add(&list, &edl_2);
    edl = edl_list_get_by_time(&list, 2);
    TEST_ASSERT(memcmp(edl, &edl_2, sizeof(edl_t)) == 0);
}

static void test_edl_list_exposure_time(void)
{
    edl_t edl_1;
    edl_t edl_2;
    edl_init_rssi(&edl_1, TEST_EDL_RSSI_1, 1);
    edl_init_rssi(&edl_2, TEST_EDL_RSSI_2, 10);
    edl_add(&list, &edl_1);
    edl_add(&list, &edl_2);
    TEST_ASSERT_EQUAL_INT(9, edl_list_exposure_time(&list));
    edl_t edl_3;
    edl_init_rssi(&edl_3, TEST_EDL_RSSI_3, 20);
    edl_add(&list, &edl_3);
    TEST_ASSERT_EQUAL_INT(19, edl_list_exposure_time(&list));
}

Test *tests_edl(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_edl_add),
        new_TestFixture(test_edl_remove),
        new_TestFixture(test_edl_list_get_nth),
        new_TestFixture(test_edl_list_get_before),
        new_TestFixture(test_edl_list_get_after),
        new_TestFixture(test_edl_list_get_by_time),
        new_TestFixture(test_edl_list_exposure_time),
    };

    EMB_UNIT_TESTCALLER(edl_tests, setUp, tearDown, fixtures);
    return (Test*)&edl_tests;
}

int main(void)
{
    TESTS_START();
    TESTS_RUN(tests_edl());
    TESTS_END();

    return 0;
}
