#include <string.h>
#include <math.h>

#include "embUnit.h"
#include "ed.h"
#include "clist.h"

#define TEST_VALUES_NUMOF       10

static uint16_t data_ts[TEST_VALUES_NUMOF] = {
    WINDOW_STEP_S - 1,      /* win 0 */
    WINDOW_STEP_S * 2 - 3,  /* win 0 & 1 */
    WINDOW_STEP_S * 2 - 2,  /* win 0 & 1 */
    WINDOW_STEP_S * 2 - 1,  /* win 0 & 1 */
    WINDOW_STEP_S * 5,      /* win 4 and 5 */
    WINDOW_STEP_S * 6 - 4,  /* win 4 and 5 */
    WINDOW_STEP_S * 6 - 3,  /* win 4 and 5  */
    WINDOW_STEP_S * 6 - 2,  /* win 4 and 5 */
    WINDOW_STEP_S * 10,     /* win 9 and 10 */
    WINDOW_STEP_S * 11,     /* win 10 and 11 */
};

static float data_rssi[TEST_VALUES_NUMOF] = {
    -80, -70, -50, -10, -20, -30, -40, -75, -25, -35,
};

static uint8_t data_slice[TEST_VALUES_NUMOF] = {
    EBID_SLICE_1, EBID_SLICE_2, EBID_SLICE_1, EBID_SLICE_2,
    EBID_XOR, EBID_XOR, EBID_XOR, EBID_SLICE_1,
    EBID_SLICE_2, EBID_SLICE_2
};

static int expected_samples[WINDOWS_PER_EPOCH] = {
    4, 3, 0, 0, 4, 4, 0, 0, 0, 1, 2, 1, 0, 0, 0
};

static float expected_avg[WINDOWS_PER_EPOCH] = {
    -16.02016, -14.77077, 000.00000, 000.00000,
    -25.56735, -25.56735, 000.00000, 000.00000,
    000.00000, -25.00000, -27.59637, -35.00000,
    000.00000, 000.00000, 000.00000
};

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

static ed_list_t list;
static ed_memory_manager_t manager;

static float _2_dec_round(float value)
{
    return round(100 * value) / 100;
}

static void setUp(void)
{
    ed_memory_manager_init(&manager);
    ed_list_init(&list, &manager);
}

static void tearDown(void)
{
    /* Finalize */
}

static void test_ed_init(void)
{
    ed_t ed;

    ed_init(&ed, 0x01);
    TEST_ASSERT(ed.cid == 0x01);
}

static void test_ed_add_remove(void)
{
    ed_t ed_1;
    ed_t ed_2;

    ed_init(&ed_1, 0x01);
    ed_init(&ed_2, 0x02);
    ed_add(&list, &ed_1);
    ed_t *first = (ed_t *)clist_rpeek(&list.list);
    TEST_ASSERT(memcmp(first, &ed_1, sizeof(ed_t)) == 0);
    ed_add(&list, &ed_2);
    first = (ed_t *)clist_rpeek(&list.list);
    TEST_ASSERT(memcmp(first, &ed_2, sizeof(ed_t)) == 0);
    ed_remove(&list, &ed_2);
    first = (ed_t *)clist_rpeek(&list.list);
    TEST_ASSERT(memcmp(first, &ed_1, sizeof(ed_t)) == 0);
    ed_remove(&list, &ed_1);
    TEST_ASSERT(clist_count(&list.list) == 0);
}

static void test_ed_list_get_nth(void)
{
    ed_t *none = ed_list_get_nth(&list, 0);

    TEST_ASSERT(none == NULL);
    ed_t ed_1;
    ed_t ed_2;
    ed_t ed_3;
    ed_init(&ed_1, 0x01);
    ed_init(&ed_2, 0x02);
    ed_init(&ed_3, 0x03);
    ed_add(&list, &ed_1);
    ed_add(&list, &ed_2);
    ed_add(&list, &ed_3);
    TEST_ASSERT(clist_count(&list.list) == 3);
    ed_t *first = ed_list_get_nth(&list, 0);
    ed_t *second = ed_list_get_nth(&list, 1);
    ed_t *third = ed_list_get_nth(&list, 2);
    TEST_ASSERT(memcmp(first, &ed_1, sizeof(ed_t)) == 0);
    TEST_ASSERT(memcmp(second, &ed_2, sizeof(ed_t)) == 0);
    TEST_ASSERT(memcmp(third, &ed_3, sizeof(ed_t)) == 0);
}

static void test_ed_list_get_by_cid(void)
{
    ed_t *none = ed_list_get_by_cid(&list, 0);

    TEST_ASSERT(none == NULL);
    ed_t ed_1;
    ed_t ed_2;
    ed_t ed_3;
    ed_init(&ed_1, 0x01);
    ed_init(&ed_2, 0x02);
    ed_init(&ed_3, 0x03);
    ed_add(&list, &ed_1);
    ed_add(&list, &ed_2);
    ed_add(&list, &ed_3);
    TEST_ASSERT(clist_count(&list.list) == 3);
    ed_t *first = ed_list_get_by_cid(&list, 0x01);
    ed_t *second = ed_list_get_by_cid(&list, 0x02);
    ed_t *third = ed_list_get_by_cid(&list, 0x03);
    TEST_ASSERT(memcmp(first, &ed_1, sizeof(ed_t)) == 0);
    TEST_ASSERT(memcmp(second, &ed_2, sizeof(ed_t)) == 0);
    TEST_ASSERT(memcmp(third, &ed_3, sizeof(ed_t)) == 0);
}

static void test_ed_process_data(void)
{
    ed_t ed;

    ed_init(&ed, 0x01);
    /* set start time since this is usually set in ed_list_process_data */
    ed.start_s = data_ts[0];
    for (uint8_t i = 0; i < TEST_VALUES_NUMOF; i++) {
        ed_process_data(&ed, data_ts[i], ebid_slice[data_slice[i]],
                        data_slice[i], data_rssi[i]);
    }
    TEST_ASSERT(ed_finish(&ed) == 0);
    for (uint8_t i = 0; i < WINDOWS_PER_EPOCH; i++) {
        TEST_ASSERT(ed.wins.wins[i].samples == expected_samples[i]);
        TEST_ASSERT(_2_dec_round(ed.wins.wins[i].avg) ==
                    _2_dec_round(expected_avg[i]));
    }
    uint16_t expected_exposure_time = data_ts[TEST_VALUES_NUMOF - 1] -
                                      data_ts[0];
    TEST_ASSERT(ed_exposure_time(&ed) == expected_exposure_time);
    TEST_ASSERT(memcmp(ebid, ed.ebid.parts.ebid.u8, EBID_SIZE) == 0);
}

static void test_ed_list_process_data(void)
{
    /* data that will be fully aggregated */
    for (uint8_t i = 0; i < TEST_VALUES_NUMOF; i++) {
        ed_list_process_data(&list, 0x01, data_ts[i],
                             ebid_slice[data_slice[i]],
                             data_slice[i], data_rssi[i]);
    }
    /* data where ebid will not be reconstructed */
    ed_list_process_data(&list, 0x02, 0, ebid_slice[0], EBID_SLICE_1,
                         -70);
    ed_list_process_data(&list, 0x02, WINDOW_STEP_S * 14, ebid_slice[0],
                         EBID_SLICE_1, -70);
    /* data where the exposure time will be insufficient */
    ed_list_process_data(&list, 0x03, 0, ebid_slice[0], EBID_SLICE_1,
                         -70);
    ed_list_process_data(&list, 0x03, WINDOW_STEP_S * 1, ebid_slice[1],
                         EBID_SLICE_2, -70);
    ed_list_process_data(&list, 0x03, WINDOW_STEP_S * 2, ebid_slice[2],
                         EBID_SLICE_3, -70);
    ed_list_finish(&list);
    TEST_ASSERT(clist_count(&list.list) == 1);
}

Test *tests_ed(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_ed_init),
        new_TestFixture(test_ed_add_remove),
        new_TestFixture(test_ed_list_get_nth),
        new_TestFixture(test_ed_list_get_by_cid),
        new_TestFixture(test_ed_process_data),
        new_TestFixture(test_ed_list_process_data),
    };

    EMB_UNIT_TESTCALLER(ed_tests, setUp, tearDown, fixtures);
    return (Test *)&ed_tests;
}

int main(void)
{
    TESTS_START();
    TESTS_RUN(tests_ed());
    TESTS_END();

    return 0;
}
