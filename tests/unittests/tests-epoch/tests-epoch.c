#include <string.h>
#include <math.h>

#include "embUnit.h"
#include "ed.h"
#include "epoch.h"
#include "clist.h"
#include "random.h"
// #include "nanocbor/nanocbor.h"

#define     TEST_CONTACTS   CONFIG_ED_BUF_SIZE

// static uint8_t buf[256];
// static uint8_t expected_cbor[] = {
//     0xd9, 0xca, 0xfe, 0x82, 0x19, 0x01, 0x4c, 0x82,
//     0x85, 0x58, 0x20, 0xbf, 0x0a, 0x8c, 0x1e, 0x3a,
//     0xe9, 0x62, 0xbb, 0xb7, 0xb3, 0x70, 0x61, 0x64,
//     0x9a, 0x8d, 0xa5, 0xdb, 0xfb, 0xc9, 0x54, 0xdc,
//     0xba, 0x4b, 0xfd, 0x8f, 0x6d, 0x8f, 0x34, 0x71,
//     0x33, 0x4a, 0x42, 0x58, 0x20, 0x50, 0x51, 0x93,
//     0x40, 0x2b, 0x31, 0xbb, 0x77, 0xfb, 0x97, 0x64,
//     0x2c, 0x2b, 0x0a, 0x67, 0x8a, 0x64, 0x96, 0xd6,
//     0xf7, 0xee, 0x04, 0x1a, 0x77, 0x0b, 0x60, 0xbc,
//     0xad, 0xd0, 0x26, 0x83, 0x5e, 0x19, 0x03, 0x0c,
//     0x19, 0x01, 0xb0, 0x18, 0x97, 0x85, 0x58, 0x20,
//     0xd8, 0x80, 0xc6, 0x76, 0x69, 0xcb, 0x97, 0x62,
//     0x43, 0x05, 0x1c, 0x5f, 0x8d, 0x5b, 0x02, 0xe4,
//     0xc3, 0x2a, 0x31, 0xd0, 0x35, 0x94, 0x68, 0xe5,
//     0xab, 0x35, 0x23, 0x9e, 0x59, 0x92, 0xf4, 0x4c,
//     0x58, 0x20, 0x10, 0x37, 0xc5, 0xc7, 0xec, 0x40,
//     0x5e, 0xbb, 0x00, 0x68, 0x82, 0x5a, 0x35, 0xb5,
//     0x20, 0x75, 0x51, 0x5f, 0xd1, 0x64, 0xe4, 0xb5,
//     0x92, 0x22, 0xc8, 0x9c, 0x33, 0x84, 0x5e, 0xdd,
//     0xa8, 0x14, 0x19, 0x02, 0x80, 0x19, 0x01, 0x43,
//     0x18, 0x47
// };

// static uint8_t expected_cbor_contact[] = {
//     0xd9, 0xca, 0xfe, 0x86, 0x19, 0x01, 0x4c, 0x58,
//     0x20, 0xbf, 0x0a, 0x8c, 0x1e, 0x3a, 0xe9, 0x62,
//     0xbb, 0xb7, 0xb3, 0x70, 0x61, 0x64, 0x9a, 0x8d,
//     0xa5, 0xdb, 0xfb, 0xc9, 0x54, 0xdc, 0xba, 0x4b,
//     0xfd, 0x8f, 0x6d, 0x8f, 0x34, 0x71, 0x33, 0x4a,
//     0x42, 0x58, 0x20, 0x50, 0x51, 0x93, 0x40, 0x2b,
//     0x31, 0xbb, 0x77, 0xfb, 0x97, 0x64, 0x2c, 0x2b,
//     0x0a, 0x67, 0x8a, 0x64, 0x96, 0xd6, 0xf7, 0xee,
//     0x04, 0x1a, 0x77, 0x0b, 0x60, 0xbc, 0xad, 0xd0,
//     0x26, 0x83, 0x5e, 0x19, 0x03, 0x0c, 0x19, 0x01,
//     0xb0, 0x18, 0x97
// };

// static const uint8_t c0_et[PET_SIZE] = {
//     0xbf, 0x0a, 0x8c, 0x1e, 0x3a, 0xe9, 0x62, 0xbb,
//     0xb7, 0xb3, 0x70, 0x61, 0x64, 0x9a, 0x8d, 0xa5,
//     0xdb, 0xfb, 0xc9, 0x54, 0xdc, 0xba, 0x4b, 0xfd,
//     0x8f, 0x6d, 0x8f, 0x34, 0x71, 0x33, 0x4a, 0x42
// };
// static const uint8_t c0_rt[PET_SIZE] = {
//     0x50, 0x51, 0x93, 0x40, 0x2b, 0x31, 0xbb, 0x77,
//     0xfb, 0x97, 0x64, 0x2c, 0x2b, 0x0a, 0x67, 0x8a,
//     0x64, 0x96, 0xd6, 0xf7, 0xee, 0x04, 0x1a, 0x77,
//     0x0b, 0x60, 0xbc, 0xad, 0xd0, 0x26, 0x83, 0x5e
// };
// static const uint8_t c1_et[PET_SIZE] = {
//     0xd8, 0x80, 0xc6, 0x76, 0x69, 0xcb, 0x97, 0x62,
//     0x43, 0x05, 0x1c, 0x5f, 0x8d, 0x5b, 0x02, 0xe4,
//     0xc3, 0x2a, 0x31, 0xd0, 0x35, 0x94, 0x68, 0xe5,
//     0xab, 0x35, 0x23, 0x9e, 0x59, 0x92, 0xf4, 0x4c
// };
// static const uint8_t c1_rt[PET_SIZE] = {
//     0x10, 0x37, 0xc5, 0xc7, 0xec, 0x40, 0x5e, 0xbb,
//     0x00, 0x68, 0x82, 0x5a, 0x35, 0xb5, 0x20, 0x75,
//     0x51, 0x5f, 0xd1, 0x64, 0xe4, 0xb5, 0x92, 0x22,
//     0xc8, 0x9c, 0x33, 0x84, 0x5e, 0xdd, 0xa8, 0x14,
// };

static void setUp(void)
{
    random_init(0);
}

static void tearDown(void)
{
    /* Finalize */
}

static void test_epoch_finish(void)
{
    ed_memory_manager_t manager;

    ed_memory_manager_init(&manager);
    crypto_manager_keys_t keys;

    crypto_manager_gen_keypair(&keys);
    epoch_data_t epoch;

    epoch_init(&epoch, 0, &keys);
    ebid_t ebid;

    ebid_generate(&ebid, &keys);
    ed_list_t list;

    ed_list_init(&list, &manager, &ebid);

    /* mock encounter data */
    for (uint8_t i = 0; i < TEST_CONTACTS; i++) {
        ed_t *ed = ed_memory_manager_calloc(list.manager);
        crypto_manager_keys_t ebid_k;
        crypto_manager_gen_keypair(&ebid_k);
        memcpy(ed->ebid.parts.ebid.u8, ebid_k.pk, EBID_SIZE);
        ed->ebid.status.status = EBID_HAS_ALL;
#if IS_USED(MODULE_ED_UWB)
        /* mock UWB data that will pass contact filter */
        ed->uwb.seen_first_s = 0;
        ed->uwb.seen_last_s = MIN_EXPOSURE_TIME_S + i;
        ed->uwb.cumulative_d_cm = (MAX_DISTANCE_CM - 1) * MIN_REQUEST_COUNT;
        ed->uwb.req_count = MIN_REQUEST_COUNT;
#endif
#if IS_USED(MODULE_ED_BLE_WIN)
        /* mock windowed BLE data that will pass contact filter */
        ed->ble_win.seen_first_s = 0;
        ed->ble_win.seen_last_s = MIN_EXPOSURE_TIME_S + i;
        for (uint8_t j = 0; j < WINDOWS_PER_EPOCH; j++) {
            ed->ble_win.wins.wins[j].samples = (uint16_t)random_uint32_range(1, 1000);
            ed->ble_win.wins.wins[j].avg = (float)random_uint32();
        }
#endif
        ed_add(&list, ed);
    }
    /* finish list */
    ed_list_finish(&list);
    TEST_ASSERT(clist_count(&list.list) == TEST_CONTACTS);
    epoch_finish(&epoch, &list);
    for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
#if IS_USED(MODULE_ED_UWB)
        TEST_ASSERT(epoch.contacts[i].uwb.exposure_s >=
                    (MIN_EXPOSURE_TIME_S + TEST_CONTACTS - CONFIG_EPOCH_MAX_ENCOUNTERS));
#endif
#if IS_USED(MODULE_ED_BLE_WIN)
        TEST_ASSERT(epoch.contacts[i].ble_win.exposure_s >=
                    (MIN_EXPOSURE_TIME_S + TEST_CONTACTS - CONFIG_EPOCH_MAX_ENCOUNTERS));
#endif
    }
    TEST_ASSERT(memarray_available(&manager.mem) == CONFIG_ED_BUF_SIZE);
}

// static void TEST_ASSER_EQUAL_UWB_CONTACT_DATA(uwb_contact_data_t *a,
//                                               uwb_contact_data_t *b)
// {
//     TEST_ASSERT_EQUAL_INT(a->avg_d_cm, b->avg_d_cm);
//     TEST_ASSERT_EQUAL_INT(a->exposure_s, b->exposure_s);
//     TEST_ASSERT_EQUAL_INT(a->req_count, b->req_count);
//     TEST_ASSERT_EQUAL_INT(0, memcmp(a->pet.et, b->pet.et, PET_SIZE));
//     TEST_ASSERT_EQUAL_INT(0, memcmp(a->pet.rt, b->pet.rt, PET_SIZE));
// }

// static void test_uwb_contact_serialize_load_cbor(void)
// {
//     uwb_contact_data_t contact = { 0 };
//     uwb_contact_data_t decoded_contact = { 0 };
//     uint32_t decoded_time = 0;

//     uint32_t time = 332;

//     contact.exposure_s = 780;
//     contact.req_count = 432;
//     contact.avg_d_cm = 151;
//     memcpy(contact.pet.et, c0_et, PET_SIZE);
//     memcpy(contact.pet.rt, c0_rt, PET_SIZE);

//     size_t len = uwb_contact_serialize_cbor(&contact, time, buf, sizeof(buf));
//     TEST_ASSERT_EQUAL_INT(0, memcmp(buf, expected_cbor_contact, len));
//     TEST_ASSERT_EQUAL_INT(0,
//                           uwb_contact_load_cbor(buf, len, &decoded_contact,
//                                                 &decoded_time));
//     TEST_ASSERT_EQUAL_INT(time, decoded_time);
//     TEST_ASSER_EQUAL_UWB_CONTACT_DATA(&contact, &decoded_contact);
// }

// static void test_epoch_serialize_load_cbor(void)
// {
//     epoch_data_t epoch;
//     crypto_manager_keys_t keys;

//     epoch_init(&epoch, 332, &keys);
//     epoch.contacts[0].exposure_s = 780;
//     epoch.contacts[0].req_count = 432;
//     epoch.contacts[0].avg_d_cm = 151;
//     memcpy(&epoch.contacts[0].pet.et, c0_et, PET_SIZE);
//     memcpy(&epoch.contacts[0].pet.rt, c0_rt, PET_SIZE);
//     epoch.contacts[1].exposure_s = 640;
//     epoch.contacts[1].req_count = 323;
//     epoch.contacts[1].avg_d_cm = 71;
//     memcpy(&epoch.contacts[1].pet.et, c1_et, PET_SIZE);
//     memcpy(&epoch.contacts[1].pet.rt, c1_rt, PET_SIZE);
//     size_t len = epoch_serialize_cbor(&epoch, buf, sizeof(buf));
//     TEST_ASSERT_EQUAL_INT(0, memcmp(buf, expected_cbor, len));
//     epoch_data_t decoded_epoch;
//     epoch_init(&decoded_epoch, 0, NULL);
//     epoch_load_cbor(buf, len, &decoded_epoch);
//     TEST_ASSER_EQUAL_UWB_CONTACT_DATA(&decoded_epoch.contacts[0],
//                                       &epoch.contacts[0]);
//     TEST_ASSER_EQUAL_UWB_CONTACT_DATA(&decoded_epoch.contacts[1],
//                                       &epoch.contacts[1]);
// }

Test *tests_epoch_all(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_epoch_finish),
        // new_TestFixture(test_uwb_contact_serialize_load_cbor),
        // new_TestFixture(test_epoch_serialize_load_cbor),
    };

    EMB_UNIT_TESTCALLER(epoch_tests, setUp, tearDown, fixtures);
    return (Test *)&epoch_tests;
}

void tests_epoch(void)
{
    TESTS_RUN(tests_epoch_all());
}
