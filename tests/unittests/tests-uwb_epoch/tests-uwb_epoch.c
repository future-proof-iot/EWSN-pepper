#include <string.h>
#include <math.h>

#include "embUnit.h"
#include "uwb_epoch.h"
#include "clist.h"
#include "random.h"

#define     TEST_CONTACTS   14

static uwb_ed_list_t list;
static uwb_ed_memory_manager_t manager;

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

Test *tests_uwb_epoch_all(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_uwb_epoch_finalize),
    };

    EMB_UNIT_TESTCALLER(uwb_epoch_tests, setUp, tearDown, fixtures);
    return (Test *)&uwb_epoch_tests;
}

void tests_uwb_epoch(void)
{
    TESTS_RUN(tests_uwb_epoch_all());
}
