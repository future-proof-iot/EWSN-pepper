#include <string.h>
#include <math.h>

#include "embUnit.h"
#include "epoch.h"
#include "clist.h"
#include "random.h"

#define     TEST_CONTACTS   14

static ed_list_t list;
static ed_memory_manager_t manager;

static void setUp(void)
{
    random_init(0);
    ed_memory_manager_init(&manager);
    ed_list_init(&list, &manager);
}

static void tearDown(void)
{
    /* Finalize */
}

static void test_epoch_init(void)
{
    epoch_data_t epoch;

    epoch_init(&epoch, 10);
    TEST_ASSERT(epoch.timestamp == 10);
}

static void test_epoch_finalize(void)
{
    epoch_data_t epoch;

    epoch_init(&epoch, 10);
    crypto_manager_keys_t keys;
    crypto_manager_gen_keypair(&keys);

    /* fake ed_list */
    for (uint8_t i = 0; i < TEST_CONTACTS; i++) {
        ed_t *ed = ed_memory_manager_calloc(list.manager);
        ed->start_s = 0;
        ed->end_s = MIN_EXPOSURE_TIME_S + i;
        crypto_manager_keys_t ebid_k;
        crypto_manager_gen_keypair(&ebid_k);
        memcpy(ed->ebid.parts.ebid.u8, ebid_k.pk, EBID_SIZE);
        ed->ebid.status.status = EBID_HAS_ALL;
        for (uint8_t j = 0; j < WINDOWS_PER_EPOCH; j++) {
            ed->wins.wins[j].samples = (uint16_t)random_uint32_range(1, 1000);
            ed->wins.wins[j].avg = (float)random_uint32();
        }
        ed_add(&list, ed);
    }
    ed_list_finish(&list);
    TEST_ASSERT(clist_count(&list.list) == TEST_CONTACTS);
    epoch_finish(&epoch, &list, &keys);
    for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
        TEST_ASSERT(epoch.contacts[i].duration >= \
                    (MIN_EXPOSURE_TIME_S + TEST_CONTACTS -
                     CONFIG_EPOCH_MAX_ENCOUNTERS));
    }
    TEST_ASSERT(memarray_available(&manager.mem) == CONFIG_ED_BUF_SIZE);
}

Test *tests_epoch_all(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_epoch_init),
        new_TestFixture(test_epoch_finalize),
    };

    EMB_UNIT_TESTCALLER(epoch_tests, setUp, tearDown, fixtures);
    return (Test *)&epoch_tests;
}

void tests_epoch(void)
{
    TESTS_RUN(tests_epoch_all());
}
