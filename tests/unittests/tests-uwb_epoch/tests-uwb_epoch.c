#include <string.h>
#include <math.h>

#include "embUnit.h"
#include "uwb_epoch.h"
#include "clist.h"
#include "random.h"
#include "nanocbor/nanocbor.h"

#define     TEST_CONTACTS   14

static uwb_ed_list_t list;
static uwb_ed_memory_manager_t manager;

static uint8_t buf[256];
static uint8_t expected_cbor[] = {
    0xd9, 0xca, 0xfe, 0x82, 0x19, 0x01, 0x4c, 0x82,
    0x85, 0x58, 0x20, 0xbf, 0x0a, 0x8c, 0x1e, 0x3a,
    0xe9, 0x62, 0xbb, 0xb7, 0xb3, 0x70, 0x61, 0x64,
    0x9a, 0x8d, 0xa5, 0xdb, 0xfb, 0xc9, 0x54, 0xdc,
    0xba, 0x4b, 0xfd, 0x8f, 0x6d, 0x8f, 0x34, 0x71,
    0x33, 0x4a, 0x42, 0x58, 0x20, 0x50, 0x51, 0x93,
    0x40, 0x2b, 0x31, 0xbb, 0x77, 0xfb, 0x97, 0x64,
    0x2c, 0x2b, 0x0a, 0x67, 0x8a, 0x64, 0x96, 0xd6,
    0xf7, 0xee, 0x04, 0x1a, 0x77, 0x0b, 0x60, 0xbc,
    0xad, 0xd0, 0x26, 0x83, 0x5e, 0x19, 0x03, 0x0c,
    0x19, 0x01, 0xb0, 0x18, 0x97, 0x85, 0x58, 0x20,
    0xd8, 0x80, 0xc6, 0x76, 0x69, 0xcb, 0x97, 0x62,
    0x43, 0x05, 0x1c, 0x5f, 0x8d, 0x5b, 0x02, 0xe4,
    0xc3, 0x2a, 0x31, 0xd0, 0x35, 0x94, 0x68, 0xe5,
    0xab, 0x35, 0x23, 0x9e, 0x59, 0x92, 0xf4, 0x4c,
    0x58, 0x20, 0x10, 0x37, 0xc5, 0xc7, 0xec, 0x40,
    0x5e, 0xbb, 0x00, 0x68, 0x82, 0x5a, 0x35, 0xb5,
    0x20, 0x75, 0x51, 0x5f, 0xd1, 0x64, 0xe4, 0xb5,
    0x92, 0x22, 0xc8, 0x9c, 0x33, 0x84, 0x5e, 0xdd,
    0xa8, 0x14, 0x19, 0x02, 0x80, 0x19, 0x01, 0x43,
    0x18, 0x47
};

static uint8_t expected_cbor_contact[] = {
    0xd9, 0xca, 0xfe, 0x86, 0x19, 0x01, 0x4c, 0x58,
    0x20, 0xbf, 0x0a, 0x8c, 0x1e, 0x3a, 0xe9, 0x62,
    0xbb, 0xb7, 0xb3, 0x70, 0x61, 0x64, 0x9a, 0x8d,
    0xa5, 0xdb, 0xfb, 0xc9, 0x54, 0xdc, 0xba, 0x4b,
    0xfd, 0x8f, 0x6d, 0x8f, 0x34, 0x71, 0x33, 0x4a,
    0x42, 0x58, 0x20, 0x50, 0x51, 0x93, 0x40, 0x2b,
    0x31, 0xbb, 0x77, 0xfb, 0x97, 0x64, 0x2c, 0x2b,
    0x0a, 0x67, 0x8a, 0x64, 0x96, 0xd6, 0xf7, 0xee,
    0x04, 0x1a, 0x77, 0x0b, 0x60, 0xbc, 0xad, 0xd0,
    0x26, 0x83, 0x5e, 0x19, 0x03, 0x0c, 0x19, 0x01,
    0xb0, 0x18, 0x97
};

static const uint8_t c0_et[PET_SIZE] = {
    0xbf, 0x0a, 0x8c, 0x1e, 0x3a, 0xe9, 0x62, 0xbb,
    0xb7, 0xb3, 0x70, 0x61, 0x64, 0x9a, 0x8d, 0xa5,
    0xdb, 0xfb, 0xc9, 0x54, 0xdc, 0xba, 0x4b, 0xfd,
    0x8f, 0x6d, 0x8f, 0x34, 0x71, 0x33, 0x4a, 0x42
};
static const uint8_t c0_rt[PET_SIZE] = {
    0x50, 0x51, 0x93, 0x40, 0x2b, 0x31, 0xbb, 0x77,
    0xfb, 0x97, 0x64, 0x2c, 0x2b, 0x0a, 0x67, 0x8a,
    0x64, 0x96, 0xd6, 0xf7, 0xee, 0x04, 0x1a, 0x77,
    0x0b, 0x60, 0xbc, 0xad, 0xd0, 0x26, 0x83, 0x5e
};
static const uint8_t c1_et[PET_SIZE] = {
    0xd8, 0x80, 0xc6, 0x76, 0x69, 0xcb, 0x97, 0x62,
    0x43, 0x05, 0x1c, 0x5f, 0x8d, 0x5b, 0x02, 0xe4,
    0xc3, 0x2a, 0x31, 0xd0, 0x35, 0x94, 0x68, 0xe5,
    0xab, 0x35, 0x23, 0x9e, 0x59, 0x92, 0xf4, 0x4c
};
static const uint8_t c1_rt[PET_SIZE] = {
    0x10, 0x37, 0xc5, 0xc7, 0xec, 0x40, 0x5e, 0xbb,
    0x00, 0x68, 0x82, 0x5a, 0x35, 0xb5, 0x20, 0x75,
    0x51, 0x5f, 0xd1, 0x64, 0xe4, 0xb5, 0x92, 0x22,
    0xc8, 0x9c, 0x33, 0x84, 0x5e, 0xdd, 0xa8, 0x14,
};

static void setUp(void)
{
    random_init(0);
    uwb_ed_memory_manager_init(&manager);
}

static void tearDown(void)
{
    /* Finalize */
}

static void test_uwb_epoch_finalize(void)
{
    uwb_epoch_data_t uwb_epoch;

    crypto_manager_keys_t keys;
    crypto_manager_gen_keypair(&keys);
    uwb_epoch_init(&uwb_epoch, 10, &keys);
    ebid_t ebid;
    ebid_generate(&ebid, &keys);
    uwb_ed_list_init(&list, &manager, &ebid);

    /* fake ed_list */
    for (uint8_t i = 0; i < TEST_CONTACTS; i++) {
        uwb_ed_t *ed = uwb_ed_memory_manager_calloc(list.manager);
        ed->seen_first_s = 0;
        ed->seen_last_s = MIN_EXPOSURE_TIME_S + i;
        crypto_manager_keys_t ebid_k;
        crypto_manager_gen_keypair(&ebid_k);
        memcpy(ed->ebid.parts.ebid.u8, ebid_k.pk, EBID_SIZE);
        ed->ebid.status.status = EBID_HAS_ALL;
        ed->cumulative_d_cm = random_uint32_range(1, 10000);
        ed->req_count = random_uint32_range(1, 100);
        uwb_ed_add(&list, ed);
    }
    uwb_ed_list_finish(&list);
    TEST_ASSERT(clist_count(&list.list) == TEST_CONTACTS);
    uwb_epoch_finish(&uwb_epoch, &list);
    for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
        TEST_ASSERT(uwb_epoch.contacts[i].exposure_s >= \
                    (MIN_EXPOSURE_TIME_S + TEST_CONTACTS -
                     CONFIG_EPOCH_MAX_ENCOUNTERS));
    }
    TEST_ASSERT(memarray_available(&manager.mem) == CONFIG_UWB_ED_BUF_SIZE);
}

static void TEST_ASSER_EQUAL_UWB_CONTACT_DATA(uwb_contact_data_t *a,
                                              uwb_contact_data_t *b)
{
    TEST_ASSERT_EQUAL_INT(a->avg_d_cm, b->avg_d_cm);
    TEST_ASSERT_EQUAL_INT(a->exposure_s, b->exposure_s);
    TEST_ASSERT_EQUAL_INT(a->req_count, b->req_count);
    TEST_ASSERT_EQUAL_INT(0, memcmp(a->pet.et, b->pet.et, PET_SIZE));
    TEST_ASSERT_EQUAL_INT(0, memcmp(a->pet.rt, b->pet.rt, PET_SIZE));
}

static void test_uwb_contact_serialize_load_cbor(void)
{
    uwb_contact_data_t contact = { 0 };
    uwb_contact_data_t decoded_contact = { 0 };
    uint32_t decoded_time = 0;

    uint32_t time = 332;

    contact.exposure_s = 780;
    contact.req_count = 432;
    contact.avg_d_cm = 151;
    memcpy(contact.pet.et, c0_et, PET_SIZE);
    memcpy(contact.pet.rt, c0_rt, PET_SIZE);

    size_t len = uwb_contact_serialize_cbor(&contact, time, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(0, memcmp(buf, expected_cbor_contact, len));
    TEST_ASSERT_EQUAL_INT(0,
                          uwb_contact_load_cbor(buf, len, &decoded_contact,
                                                &decoded_time));
    TEST_ASSERT_EQUAL_INT(time, decoded_time);
    TEST_ASSER_EQUAL_UWB_CONTACT_DATA(&contact, &decoded_contact);
}

static void test_uwb_epoch_serialize_load_cbor(void)
{
    uwb_epoch_data_t epoch;
    crypto_manager_keys_t keys;

    uwb_epoch_init(&epoch, 332, &keys);
    epoch.contacts[0].exposure_s = 780;
    epoch.contacts[0].req_count = 432;
    epoch.contacts[0].avg_d_cm = 151;
    memcpy(&epoch.contacts[0].pet.et, c0_et, PET_SIZE);
    memcpy(&epoch.contacts[0].pet.rt, c0_rt, PET_SIZE);
    epoch.contacts[1].exposure_s = 640;
    epoch.contacts[1].req_count = 323;
    epoch.contacts[1].avg_d_cm = 71;
    memcpy(&epoch.contacts[1].pet.et, c1_et, PET_SIZE);
    memcpy(&epoch.contacts[1].pet.rt, c1_rt, PET_SIZE);
    size_t len = uwb_epoch_serialize_cbor(&epoch, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(0, memcmp(buf, expected_cbor, len));
    uwb_epoch_data_t decoded_epoch;
    uwb_epoch_init(&decoded_epoch, 0, NULL);
    uwb_epoch_load_cbor(buf, len, &decoded_epoch);
    TEST_ASSER_EQUAL_UWB_CONTACT_DATA(&decoded_epoch.contacts[0],
                                      &epoch.contacts[0]);
    TEST_ASSER_EQUAL_UWB_CONTACT_DATA(&decoded_epoch.contacts[1],
                                      &epoch.contacts[1]);
}

Test *tests_uwb_epoch_all(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_uwb_epoch_finalize),
        new_TestFixture(test_uwb_contact_serialize_load_cbor),
        new_TestFixture(test_uwb_epoch_serialize_load_cbor),
    };

    EMB_UNIT_TESTCALLER(uwb_epoch_tests, setUp, tearDown, fixtures);
    return (Test *)&uwb_epoch_tests;
}

void tests_uwb_epoch(void)
{
    TESTS_RUN(tests_uwb_epoch_all());
}
