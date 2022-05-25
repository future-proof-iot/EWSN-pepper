/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_desire_ble_adv
 * @{
 *
 * @file
 * @brief       DESIRE based ble advertisement module
 *
 * @author      Roudy Dagher <roudy.dagher@inria.fr>
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include "nimble_riot.h"
#include "host/ble_gap.h"

#include "net/bluetil/ad.h"

#include "desire_ble_adv.h"
#include "desire/ble_pkt.h"
#include "ble_pkt_dbg.h"

#include "event.h"
#include "event/callback.h"
#include "event/timeout.h"
#include "event/thread.h"
#include "random.h"
#include "ztimer.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_ERROR
#endif
#include "log.h"

/**
 * @brief The extended advertisement event duration
 *
 * This value should not matter, it will timeout because of the amount
 * of configured event
 */
#define ADV_DURATION_MS 1000

#if IS_USED(MODULE_DESIRE_ADVERTISER_THREADED)
static char _stack[CONFIG_BLE_ADV_THREAD_STACKSIZE];
static event_queue_t _threaded_queue;
#endif
/* pointer to the event queue to handle the advertisement events */
static event_queue_t *_queue;

/* settings for advertising procedure */
static struct ble_gap_ext_adv_params _ext_advp;
/* buffer for _ad */
static uint8_t buf[BLE_HS_ADV_MAX_SZ];
/* advertising data struct */
static bluetil_ad_t _ad;

/* advertisement callback */
static ble_adv_cb_t _adv_cb = NULL;
static void *_adv_cb_arg = NULL;

/**
 * @brief   Advertising managemer struct
 */
typedef struct {
    desire_ble_adv_payload_t ble_adv_payload;   /**< BLE Advertisement: Service Data Payload */
    ebid_t *ebid;                               /**< Current EBID information */
    uint32_t cid;                               /**< The connection id */
    uint32_t itvl_ms;                           /**< the advertisement interval in ms */
    uint32_t advs;                              /**< current event count */
    uint32_t advs_max;                          /**< the total amount of advertisements */
    uint32_t advs_slice;                        /**< number of advertisements per slice */
    uint16_t seed;                              /**< random seed value */
} adv_mgr_t;
static adv_mgr_t _adv_mgr;

/**
 * @brief   Advertising event
 */
typedef struct {
    event_timeout_t timeout;    /**< the event timeout */
    event_callback_t event;     /**< the event callback */
    adv_mgr_t *mgr;             /**< the advertsing manager */
} adv_event_t;
static adv_event_t _adv_event;

static int _gap_event_cb(struct ble_gap_event *event, void *arg)
{
    adv_mgr_t *mgr = (adv_mgr_t *)arg;

    switch (event->type) {
    case BLE_GAP_EVENT_ADV_COMPLETE:
        LOG_DEBUG("[adv] %" PRIu32 " : complete event\n", ztimer_now(ZTIMER_MSEC));
        if (_adv_cb) {
            /* timestamping at this stage does not make sense, there is in any
               case already an OS delay of at least 100us */
            _adv_cb(mgr->seed, _adv_cb_arg);
        }
        break;
    default:
        LOG_WARNING("[adv] warning: unhandled event %" PRIu8 "\n", event->type);
        break;
    }

    return 0;
}

static void _set_ble_adv_address(void)
{
    /* set random (NRPA) address for instance */
    ble_addr_t addr;
    int rc;

    rc = ble_hs_id_gen_rnd(1, &addr);
    assert(rc == 0);
    rc = ble_gap_ext_adv_set_addr(CONFIG_DESIRE_ADV_INST, &addr);
    assert(rc == 0);
    (void)rc;
}

static void _configure_ext_adv(void)
{
    int rc;

    (void)rc;
    memset(&_ext_advp, 0, sizeof(_ext_advp));
    /* legacy PDUs do not support higher data rate */
    _ext_advp.primary_phy = BLE_HCI_LE_PHY_1M;
    _ext_advp.secondary_phy = BLE_HCI_LE_PHY_1M;
    /* TODO: check if this is the actual preffered tx power, not sure how
       it was handled before */
    _ext_advp.tx_power = CONFIG_BLE_ADV_TX_POWER;
    /* sid is different from slice id using in desire_ble_pkt */
    _ext_advp.sid = CONFIG_DESIRE_ADV_INST;
    /* use legacy PDUs */
    _ext_advp.legacy_pdu = 1;
    /* advertise using random addr */
    _ext_advp.own_addr_type = BLE_OWN_ADDR_RANDOM;

    rc = ble_gap_ext_adv_configure(CONFIG_DESIRE_ADV_INST, &_ext_advp,
                                   NULL, _gap_event_cb, &_adv_mgr);
    assert(rc == 0);
    /* set address */
    _set_ble_adv_address();
}

static void _set_adv_data(desire_ble_adv_payload_t *adv_payload)
{
    bluetil_ad_init(&_ad, buf, 0, sizeof(buf));
    /* Tx power field added by the driver */
    int8_t phy_txpwr_dbm = ble_current_tx_pwr();
    int rc = bluetil_ad_add(&_ad, BLE_GAP_AD_TX_POWER_LEVEL, &phy_txpwr_dbm,
                            sizeof(phy_txpwr_dbm));

    assert(rc == BLUETIL_AD_OK);

    /* Add service data uuid */
    uint16_t service_data = DESIRE_SERVICE_UUID16;

    rc = bluetil_ad_add(&_ad, BLE_GAP_AD_UUID16_COMP, &service_data,
                        sizeof(service_data));
    assert(rc == BLUETIL_AD_OK);

    /* Add service data field */
    rc = bluetil_ad_add(&_ad, BLE_GAP_AD_SERVICE_DATA_UUID16,
                        adv_payload->bytes,  DESIRE_ADV_PAYLOAD_SIZE);
    assert(rc == BLUETIL_AD_OK);
    (void)rc;
}

static void _advertise_once(desire_ble_adv_payload_t *adv_payload)
{
    int rc;

    (void)rc;

    if (ble_gap_ext_adv_active(CONFIG_DESIRE_ADV_INST)) {
        rc = ble_gap_ext_adv_stop(CONFIG_DESIRE_ADV_INST);
        assert(rc == BLE_HS_EALREADY || rc == 0);
    }

    /* get mbuf for adv data, this is freed from the `ble_gap_ext_adv_set_data` */
    struct os_mbuf *data;

    data = os_msys_get_pkthdr(BLE_HS_ADV_MAX_SZ, 0);
    assert(data);

    /* update adv payload */
    _set_adv_data(adv_payload);

    /* fill mbuf with adv data */
    rc = os_mbuf_append(data, _ad.buf, _ad.pos);
    assert(rc == 0);
    /* set adv data */
    rc = ble_gap_ext_adv_set_data(CONFIG_DESIRE_ADV_INST, data);
    assert(rc == 0);

    /* set a single advertisement event */
    LOG_DEBUG("[adv]: ext_adv start");
    rc = ble_gap_ext_adv_start(CONFIG_DESIRE_ADV_INST, ADV_DURATION_MS / 10, 1);
    assert(rc == 0);
}

static void _ebid_slice_rotate(adv_mgr_t *mgr)
{
    uint8_t slice[EBID_SLICE_SIZE_LONG];
    uint8_t slice_id = ((mgr->advs / mgr->advs_slice) % EBID_PARTS);

    LOG_DEBUG("[adv]: rotate to slice %" PRIu8 "\n", slice_id);
    switch (slice_id) {
    case EBID_SLICE_1:
    case EBID_SLICE_2:
    case EBID_XOR:
        memcpy(slice, ebid_get_slice(mgr->ebid, slice_id), EBID_SLICE_SIZE_LONG);
        break;
    case EBID_SLICE_3:
        /* DESIRE sends the third slice with front padding so ignore first 4 bytes:
           https://gitlab.inria.fr/aboutet1/test-bluetooth/-/blob/master/app/src/main/java/fr/inria/desire/ble/models/AdvPayload.kt#L54
         */
        memset(slice, '\0', EBID_SLICE_SIZE_LONG - EBID_SLICE_SIZE_SHORT);
        memcpy(slice + EBID_SLICE_SIZE_LONG - EBID_SLICE_SIZE_SHORT, ebid_get_slice3(mgr->ebid), EBID_SLICE_SIZE_SHORT);
        break;
    default:
        assert(false);
        LOG_ERROR("[adv] error: this should never happen\n");
        break;
    }
    if (IS_ACTIVE(CONFIG_BLE_ADV_SEED_RANDOM)) {
        mgr->seed = (uint16_t)random_uint32();
    }
    else {
        mgr->seed = (uint16_t)mgr->advs;
    }
    desire_ble_adv_payload_build(&mgr->ble_adv_payload, slice_id, mgr->cid, slice, mgr->seed);
}

static void _adv_mgr_init(ebid_t *ebid, uint32_t itvl_ms, uint32_t advs_max,
                          uint32_t advs_slice)
{
    /* generate a random CID that remains the same for the given EBID during
       the advertisement period */
    if (!IS_ACTIVE(CONFIG_BLE_ADV_STATIC_CID)) {
        _adv_mgr.cid = desire_ble_adv_gen_cid();
    }
    _adv_mgr.ebid = ebid;
    _adv_mgr.advs = 0;
    _adv_mgr.advs_max = advs_max;
    _adv_mgr.advs_slice = advs_slice;
    _adv_mgr.itvl_ms = itvl_ms;
    /* build initial payload */
    _ebid_slice_rotate(&_adv_mgr);
}

static void _desire_advertiser_handler(void *arg)
{
    adv_event_t *mgr_event = (adv_event_t *)arg;
    adv_mgr_t *mgr = mgr_event->mgr;

    LOG_DEBUG("[adv]: tick\n");
    mgr->advs++;
    /* re-arm timeout if needed, advs remaining or advs forever */
    if (mgr->advs != mgr->advs_max || mgr->advs_max == BLE_ADV_TIMEOUT_NEVER) {
        LOG_DEBUG("[adv]: set next advertisement in %" PRIu32 "\n", mgr->itvl_ms);
        event_timeout_set(&mgr_event->timeout, mgr->itvl_ms);
    }
    else {
        /* the last advertisement might not yet be out, depends on OS delays */
        LOG_DEBUG("[adv]: last adv of %" PRIu32 " advertisements\n", mgr->advs);
    }

    /* rotate payload if needed, this applies to next advertisement */
    if (mgr->advs % mgr->advs_slice == 0) {
        _ebid_slice_rotate(mgr);
    }

    /* advertise once */
    _advertise_once(&mgr->ble_adv_payload);
}

void desire_ble_adv_init(event_queue_t *queue)
{
    _queue = queue;
    /* init adv event */
    event_timeout_ztimer_init(&_adv_event.timeout, ZTIMER_MSEC, _queue,
                              &_adv_event.event.super);
    event_callback_init(&_adv_event.event, _desire_advertiser_handler,
                        &_adv_event);
    _adv_event.mgr = &_adv_mgr;
    /* configure advertisement base parameters */
    _configure_ext_adv();
}

#if IS_USED(MODULE_DESIRE_ADVERTISER_THREADED)
void desire_ble_adv_init_threaded(void)
{
    /* init event queue */
    event_queue_init(&_threaded_queue);
    desire_ble_adv_init(&_threaded_queue);
    /* thread that will run an event loop (event_loop) advertisement events */
    event_thread_init(&_threaded_queue, _stack, sizeof(_stack),
                      CONFIG_BLE_ADV_THREAD_PRIO);
}
#endif

void desire_ble_adv_stop(void)
{
    LOG_DEBUG("[adv]: stop adv\n");
    if (ble_gap_ext_adv_active(CONFIG_DESIRE_ADV_INST)) {
        int rc = ble_gap_ext_adv_stop(CONFIG_DESIRE_ADV_INST);
        (void)rc;
        assert(rc == BLE_HS_EALREADY || rc == 0);
    }
    event_timeout_clear(&_adv_event.timeout);
}

void desire_ble_adv_start(ebid_t *ebid, adv_params_t *params)
{
    LOG_DEBUG("[adv]: start adv\n");
    /* stop ongoing advertisements if any */
    desire_ble_adv_stop();
    /* set a new NRPA ble address */
    _set_ble_adv_address();
    /* setup advertisement parameters */
    _adv_mgr_init(ebid, params->itvl_ms, params->advs_max, params->advs_slice);
    /* setup first advertisement event */
    event_timeout_set(&_adv_event.timeout, params->itvl_ms);
}

uint32_t desire_ble_adv_get_cid(void)
{
    return _adv_mgr.cid;
}

uint32_t desire_ble_adv_gen_cid(void)
{
    return random_uint32() & MASK_CID;
}

void desire_ble_adv_set_cid(uint32_t cid)
{
    _adv_mgr.cid = cid;
}

void desire_ble_adv_set_cb(ble_adv_cb_t cb)
{
    _adv_cb = cb;
}

void desire_ble_adv_set_cb_arg(void *arg)
{
    _adv_cb_arg = arg;
}

void desire_ble_adv_status_print(void)
{
    printf("[adv] status:\n");
    dbg_dump_buffer("\t ebid = ", ebid_get(_adv_mgr.ebid), EBID_SIZE, '\n');
    uint8_t slice_id = ((_adv_mgr.advs / _adv_mgr.advs_slice) % EBID_PARTS);

    printf("\t cid = %" PRIx32 ", sid = %d\n", _adv_mgr.cid, slice_id);
    dbg_dump_buffer("\t slice = ", ebid_get_slice(_adv_mgr.ebid, slice_id),
                    EBID_SLICE_SIZE_LONG, '\n');
    dbg_dump_buffer("\t adv_payload = ", _adv_mgr.ble_adv_payload.bytes,
                    DESIRE_ADV_PAYLOAD_SIZE, '\n');
    printf("\t itvl_ms = %" PRIu32 ", advs_slice = %" PRIu32 "\n", _adv_mgr.itvl_ms,
           _adv_mgr.advs_slice);
    printf("\t advs = %" PRIu32 "/%" PRIu32 "\n", _adv_mgr.advs, _adv_mgr.advs_max);
}
