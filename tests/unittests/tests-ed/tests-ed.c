#include <string.h>
#include <math.h>

#include "embUnit.h"
#include "ed.h"
#include "clist.h"

#define TEST_VALUES_NUMOF       10

#if IS_USED(MODULE_ED_BLE) || (MODULE_ED_BLE_WIN)
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
    -72, -70, -50, -10, -20, -30, -40, -75, -25, -35,
};

#if (MODULE_ED_BLE_WIN)
static int expected_samples[WINDOWS_PER_EPOCH] = {
    4, 3, 0, 0, 4, 4, 0, 0, 0, 1, 2, 1, 0, 0, 0
};

static float expected_avg[WINDOWS_PER_EPOCH] = {
    -16.02016, -14.77077, 000.00000, 000.00000,
    -25.56735, -25.56735, 000.00000, 000.00000,
    000.00000, -25.00000, -27.59637, -35.00000,
    000.00000, 000.00000, 000.00000
};
#endif

#if IS_USED(MODULE_ED_BLE)
static float expected_avg_all = -19.40858;
#endif
#endif

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

static ed_list_t list;
static ed_memory_manager_t manager;
static ebid_t local_ebid;

#if IS_USED(MODULE_ED_BLE) || (MODULE_ED_BLE_WIN)
static float _2_dec_round(float value)
{
    return truncf(value * 100);
}
#endif

static void setUp(void)
{
    ebid_init(&local_ebid);
    memcpy(local_ebid.parts.ebid.u8, local_ebid_1, EBID_SIZE);
    local_ebid.status.status = EBID_HAS_ALL;
    ed_memory_manager_init(&manager);
    ed_list_init(&list, &manager, &local_ebid);
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

static void test_ed_list_get_by_short_addr(void)
{
    ed_t *none = ed_list_get_by_short_addr(&list, 0);

    TEST_ASSERT(none == NULL);
    ed_t ed_1;
    ed_t ed_2;

    ed_init(&ed_1, 0x12345678);
    ed_init(&ed_2, 0x56781234);
    ed_add(&list, &ed_1);
    ed_add(&list, &ed_2);
    TEST_ASSERT(clist_count(&list.list) == 2);
    ed_t *first =  ed_list_get_by_short_addr(&list, ed_get_short_addr(&ed_1));
    ed_t *second = ed_list_get_by_short_addr(&list, ed_get_short_addr(&ed_2));

    TEST_ASSERT(memcmp(first, &ed_1, sizeof(ed_t)) == 0);
    TEST_ASSERT(memcmp(second, &ed_2, sizeof(ed_t)) == 0);
}

static void test_ed_add_slice(void)
{
    ed_t ed;

    ed_init(&ed, 0x01);
    ed_add_slice(&ed, 0, ebid_slice[0], EBID_SLICE_1);
    ed_add_slice(&ed, 0, ebid_slice[1], EBID_SLICE_2);
    uint8_t pad_slice3[EBID_SLICE_SIZE_LONG];

    memset(pad_slice3, '\0', EBID_SLICE_SIZE_LONG);
    memcpy(pad_slice3 + EBID_SLICE_SIZE_PAD, ebid_slice[2], EBID_SLICE_SIZE_SHORT);
    ed_add_slice(&ed, 0, pad_slice3, EBID_SLICE_3);
    TEST_ASSERT(memcmp(ebid, ed.ebid.parts.ebid.u8, EBID_SIZE) == 0);
}

static void test_ed_list_process_slice(void)
{
    ed_t *ed_0;
    ed_t *ed_1;

    TEST_ASSERT(clist_count(&list.list) == 0);
    for (size_t i = 0; i < CONFIG_ED_BUF_SIZE; i++) {
        ed_0 = ed_list_process_slice(&list, i, 0, ebid_slice[2], EBID_SLICE_3);
        TEST_ASSERT(clist_count(&list.list) == (i + 1));
        ed_1 = ed_list_process_slice(&list, i, 0, ebid_slice[1], EBID_SLICE_2);
        TEST_ASSERT(clist_count(&list.list) == (i + 1));
        TEST_ASSERT(ed_0 == ed_1);
    }
    ed_0 = ed_list_process_slice(&list, CONFIG_ED_BUF_SIZE, 0, ebid_slice[2], EBID_SLICE_3);
    TEST_ASSERT(!ed_0);
}

static void _ed_uwb_valid_init(ed_t *ed)
{
#if IS_USED(MODULE_ED_UWB)
    ed->uwb.seen_first_s = 0;
    ed->uwb.seen_last_s = MIN_EXPOSURE_TIME_S;
    ed->uwb.req_count = MIN_REQUEST_COUNT;
    ed->uwb.cumulative_d_cm = (MAX_DISTANCE_CM - 1) * MIN_REQUEST_COUNT;
#else
    (void)ed;
#endif
}

#if IS_USED(MODULE_ED_UWB)
static void test_ed_uwb_process_data(void)
{
    ed_t ed;

    ed_init(&ed, 0x01);
    ed_uwb_process_data(&ed, 10, 200, 0, 0);
    ed_uwb_process_data(&ed, 40, 100, 0, 0);
    ed_uwb_process_data(&ed, 300, 50, 0, 0);
    ed_uwb_process_data(&ed, MIN_EXPOSURE_TIME_S, 50, 0, 0);
    TEST_ASSERT_EQUAL_INT(400, ed.uwb.cumulative_d_cm);
    TEST_ASSERT_EQUAL_INT(MIN_EXPOSURE_TIME_S, ed.uwb.seen_last_s);
    TEST_ASSERT_EQUAL_INT(4, ed.uwb.req_count);
}

static void test_ed_uwb_finish(void)
{
    ed_t ed;
    ed_init(&ed, 0x01);
    ed.uwb.seen_first_s = 0;
    ed.uwb.seen_last_s = MIN_EXPOSURE_TIME_S - 1;
    TEST_ASSERT(ed_uwb_finish(&ed, MIN_EXPOSURE_TIME_S) == false);
    ed.uwb.seen_last_s = MIN_EXPOSURE_TIME_S;
    ed.uwb.req_count = MIN_REQUEST_COUNT - 1;
    TEST_ASSERT(ed_uwb_finish(&ed, MIN_EXPOSURE_TIME_S) == false);
    ed.uwb.req_count = 4;
    ed.uwb.cumulative_d_cm = (MAX_DISTANCE_CM + 1) * 4;
    TEST_ASSERT(ed_uwb_finish(&ed, MIN_EXPOSURE_TIME_S) == false);
    ed.uwb.cumulative_d_cm = (MAX_DISTANCE_CM - 1) * 4;
    TEST_ASSERT(ed_uwb_finish(&ed, MIN_EXPOSURE_TIME_S) == true);
    TEST_ASSERT(ed.uwb.valid == true);
}

static void test_ed_list_process_rng_data(void)
{
    ed_t *ed;

    TEST_ASSERT(!ed_list_process_rng_data(&list, 0x00, 0, 100, 0, 0));
    ed = ed_list_process_slice(&list, 0x00, 0, ebid_slice[2], EBID_SLICE_3);
    TEST_ASSERT(ed_list_process_rng_data(&list, 0x00, 0, 100, 0, 0) == ed);
}
#endif

#if IS_USED(MODULE_ED_BLE) || IS_USED(MODULE_ED_BLE_WIN)
static void test_ed_set_obf_value(void)
{
    ed_t ed;

    ed_init(&ed, 0x01);
    memcpy(ed.ebid.parts.ebid.u8, ebid, EBID_SIZE);
    ed.ebid.status.status = EBID_HAS_ALL;
    ed_ble_set_obf_value(&ed, &local_ebid);
    TEST_ASSERT(ed.obf == (0x5c24 % CONFIG_ED_BLE_OBFUSCATE_MAX));
}
#endif

static void _ed_ble_win_valid_init(ed_t *ed)
{
#if IS_USED(MODULE_ED_UWB)
    ed->ble_win.seen_first_s = 0;
    ed->ble_win.seen_last_s = MIN_EXPOSURE_TIME_S;
#else
    (void)ed;
#endif
}

#if IS_USED(MODULE_ED_BLE) || (MODULE_ED_BLE_WIN)
static void test_ed_list_process_slice_obf(void)
{
    ed_t *ed_0;

    TEST_ASSERT(clist_count(&list.list) == 0);
    ed_0 = ed_list_process_slice(&list, 0, 0, ebid_slice[2], EBID_SLICE_3);
    TEST_ASSERT(clist_count(&list.list) == 1);
    TEST_ASSERT(ed_0->obf == 0x0000);
    ed_list_process_slice(&list, 0, 0, ebid_slice[1], EBID_SLICE_2);
    ed_list_process_slice(&list, 0, 0, ebid_slice[0], EBID_SLICE_1);
    TEST_ASSERT(ed_0->obf != 0x0000);
}
#endif

#if IS_USED(MODULE_ED_BLE_WIN)
static void test_ed_ble_win_process_data(void)
{
    ed_t ed;

    ed_init(&ed, 0x01);
    /* set start time since this is usually set in ed_list_process_data */
    ed.ble_win.seen_first_s = data_ts[0];
    /* don't offuscate data, easier to test */
    ed.obf = 0;
    for (uint8_t i = 0; i < TEST_VALUES_NUMOF; i++) {
        ed_ble_win_process_data(&ed, data_ts[i], data_rssi[i]);
    }
    TEST_ASSERT(ed_ble_win_finish(&ed, MIN_EXPOSURE_TIME_S) == true);
    for (uint8_t i = 0; i < WINDOWS_PER_EPOCH; i++) {
        TEST_ASSERT(ed.ble_win.wins.wins[i].samples == expected_samples[i]);
        TEST_ASSERT(_2_dec_round(ed.ble_win.wins.wins[i].avg) ==
                    _2_dec_round(expected_avg[i]));
    }
    uint16_t expected_exposure_time = data_ts[TEST_VALUES_NUMOF - 1] -
                                      data_ts[0];

    TEST_ASSERT((ed.ble_win.seen_last_s - ed.ble_win.seen_first_s) == expected_exposure_time);
}

static void test_ed_ble_win_finish(void)
{
    ed_t ed;
    ed_init(&ed, 0x01);
    ed.ble_win.seen_first_s = 0;
    ed.ble_win.seen_last_s = MIN_EXPOSURE_TIME_S - 1;
    TEST_ASSERT(ed_ble_win_finish(&ed, MIN_EXPOSURE_TIME_S) == false);
    ed.ble_win.seen_last_s = MIN_EXPOSURE_TIME_S;
    TEST_ASSERT(ed_ble_win_finish(&ed, MIN_EXPOSURE_TIME_S) == true);
}
#endif

#if IS_USED(MODULE_ED_BLE)
static void test_ed_ble_process_data(void)
{
    ed_t ed;

    ed_init(&ed, 0x01);
    /* set start time since this is usually set in ed_list_process_data */
    ed.ble.seen_first_s = data_ts[0];
    /* don't offuscate data, easier to test */
    ed.obf = 0;
    for (uint8_t i = 0; i < TEST_VALUES_NUMOF; i++) {
        ed_ble_process_data(&ed, data_ts[i], data_rssi[i]);
    }
    TEST_ASSERT(ed_ble_finish(&ed, MIN_EXPOSURE_TIME_S) == true);
    TEST_ASSERT(_2_dec_round(ed.ble.cumulative_rssi) ==
                _2_dec_round(expected_avg_all));
    uint16_t expected_exposure_time = data_ts[TEST_VALUES_NUMOF - 1] - data_ts[0];
    TEST_ASSERT((ed.ble.seen_last_s - ed.ble.seen_first_s) == expected_exposure_time);
}

static void test_ed_ble_finish(void)
{
    ed_t ed;
    ed_init(&ed, 0x01);
    ed.ble.seen_first_s = 0;
    ed.ble.seen_last_s = MIN_EXPOSURE_TIME_S - 1;
    TEST_ASSERT(ed_ble_finish(&ed, MIN_EXPOSURE_TIME_S) == false);
    ed.ble.scan_count = 1;
    ed.ble.cumulative_rssi = pow(10.0, -90 / 10.0);;
    ed.ble.seen_last_s = MIN_EXPOSURE_TIME_S;
    TEST_ASSERT(ed_ble_finish(&ed, MIN_EXPOSURE_TIME_S) == false);
    ed.ble.scan_count = 1;
    ed.ble.cumulative_rssi = pow(10.0, -30 / 10.0);;
    TEST_ASSERT(ed_ble_finish(&ed, MIN_EXPOSURE_TIME_S) == true);
}
#endif


static void test_ed_finish(void)
{
    ed_t ed;
    ed_init(&ed, 0x01);
    TEST_ASSERT(ed_finish(&ed, MIN_EXPOSURE_TIME_S) == false);
    if (IS_USED(MODULE_ED_UWB)) {
        ed_init(&ed, 0x01);
        _ed_uwb_valid_init(&ed);
        TEST_ASSERT(ed_finish(&ed, MIN_EXPOSURE_TIME_S) == true);
    }
    if (IS_USED(MODULE_ED_BLE_WIN)) {
        ed_init(&ed, 0x01);
        _ed_ble_win_valid_init(&ed);
        TEST_ASSERT(ed_finish(&ed, MIN_EXPOSURE_TIME_S) == true);
        TEST_ASSERT(ed_finish(&ed, MIN_EXPOSURE_TIME_S) == true);
    }
    if (IS_USED(MODULE_ED_BLE_WIN) && IS_USED(MODULE_ED_BLE_WIN)) {
        ed_init(&ed, 0x01);
        _ed_uwb_valid_init(&ed);
        TEST_ASSERT(ed_finish(&ed, MIN_EXPOSURE_TIME_S) == true);
        _ed_ble_win_valid_init(&ed);
        TEST_ASSERT(ed_finish(&ed, MIN_EXPOSURE_TIME_S) == true);
    }
}

static void test_ed_list_finish(void)
{
    ed_t *ed_0;
    ed_t *ed_1;
    ed_t *ed_2;

    /* add the cids to the list, EBID reconstruction is not checked at
       this stage */
    TEST_ASSERT(clist_count(&list.list) == 0);
    ed_0 = ed_list_process_slice(&list, 0x00, 0, ebid_slice[2], EBID_SLICE_3);
    ed_1 = ed_list_process_slice(&list, 0x01, 0, ebid_slice[1], EBID_SLICE_2);
    ed_2 = ed_list_process_slice(&list, 0x02, 0, ebid_slice[0], EBID_SLICE_1);
    TEST_ASSERT(clist_count(&list.list) == 3);
    /* encounter data with no valid field */
    (void)ed_0;
    /* encounter data with at least one valid field */
    if (IS_USED(MODULE_ED_UWB)) {
        _ed_uwb_valid_init(ed_1);
    }
    /* encounter data with all valid field */
    if (IS_USED(MODULE_ED_UWB)) {
        _ed_uwb_valid_init(ed_2);
        _ed_ble_win_valid_init(ed_2);
    }
    /* finish list processing, two should remain */
    ed_list_finish(&list);
    TEST_ASSERT_EQUAL_INT(2, clist_count(&list.list));
}

Test *tests_ed_all(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_ed_init),
        new_TestFixture(test_ed_add_remove),
        new_TestFixture(test_ed_add_slice),
        new_TestFixture(test_ed_list_get_nth),
        new_TestFixture(test_ed_list_get_by_cid),
        new_TestFixture(test_ed_list_get_by_short_addr),
        new_TestFixture(test_ed_list_process_slice),
#if IS_USED(MODULE_ED_BLE) || IS_USED(MODULE_ED_BLE_WIN)
        new_TestFixture(test_ed_set_obf_value),
        new_TestFixture(test_ed_list_process_slice_obf),
#endif
#if IS_USED(MODULE_ED_BLE_WIN)
        new_TestFixture(test_ed_ble_win_process_data),
        new_TestFixture(test_ed_ble_win_finish),
#endif
#if IS_USED(MODULE_ED_BLE)
        new_TestFixture(test_ed_ble_process_data),
        new_TestFixture(test_ed_ble_finish),
#endif
#if IS_USED(MODULE_ED_UWB)
        new_TestFixture(test_ed_uwb_process_data),
        new_TestFixture(test_ed_uwb_finish),
        new_TestFixture(test_ed_list_process_rng_data),
#endif
        new_TestFixture(test_ed_finish),
        new_TestFixture(test_ed_list_finish),
    };

    EMB_UNIT_TESTCALLER(ed_tests, setUp, tearDown, fixtures);
    return (Test *)&ed_tests;
}

void tests_ed(void)
{
    TESTS_RUN(tests_ed_all());
}
