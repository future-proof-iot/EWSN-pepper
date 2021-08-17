#include "desire_ble_adv.h"

#include "ble_pkt_dbg.h"

#include "event.h"
#include "event/timeout.h"
#include "event/thread.h"

#include "random.h"
#include "assert.h"

#include "ztimer.h"

#include "net/bluetil/ad.h"
#include "nimble/hci_common.h"

#include "nimble_autoadv.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

#define ENABLE_DEBUG    0
#include "debug.h"

/* BLE Advetisement */
#define BLE_NIMBLE_ADV_DURATION_MS 10
static void ble_advertise_once(desire_ble_adv_payload_t *adv_payload);

// Ticker Event thread
#define TICK_EVENT_INTERVAL (1 * MS_PER_SEC)
/* pointer to the event queue to handle the advertisement events */
static event_queue_t* _queue;

static void _tick_event_handler(event_t *e);
static event_t _update_evt = {
    .handler = _tick_event_handler
};
static event_timeout_t _update_timeout_evt;

static ble_adv_cb_t _user_adv_cb = NULL;

// EBID generation and slicing management
typedef struct {
    /* Current EBID information */
    ebid_t ebid;
    uint32_t cid;
    uint8_t sid;
    uint8_t *current_ebid_slice;
    /* BLE Advertisement: Service Data Payload */
    desire_ble_adv_payload_t ble_adv_payload;
    /* Time counters */
    struct {
        uint16_t slice_adv_time_sec;
        uint16_t ebid_adv_time_sec;
    } limits;
    struct {
        uint32_t slice;
        uint32_t ebid;
    } ticks;
} ebid_mgr_t;
static ebid_mgr_t ebid_mgr;

static void dbg_print_ebid_mgr(void);

static void _ebid_mgr_init(ebid_t *ebid, uint16_t slice_adv_time_sec,
                           uint16_t ebid_adv_time_sec);
static bool _ebid_mgr_tick(void);

#if IS_USED(MODULE_DESIRE_ADVERTISER_THREADED)
static char _stack[THREAD_STACKSIZE_DEFAULT];
static event_queue_t _threaded_queue;
void desire_ble_adv_init_threaded(void)
{
    /* init event queue */
    event_queue_init(&_threaded_queue);
    desire_ble_adv_init(&_threaded_queue);
    /* Thread that will run an event loop (event_loop) for handling
       TICK_EVENT_INTERVAL second tick */
    event_thread_init(&_threaded_queue, _stack, sizeof(_stack),
                      EVENT_QUEUE_PRIO_HIGHEST);
}
#endif

void desire_ble_adv_init(event_queue_t *queue)
{
    _queue = queue;
    _user_adv_cb = NULL;
    event_timeout_ztimer_init(&_update_timeout_evt, ZTIMER_MSEC, _queue,
                              &_update_evt);
}

void desire_ble_adv_start(ebid_t *ebid,
                          uint16_t slice_adv_time_sec,
                          uint16_t ebid_adv_time_sec)
{
    // stop current advetisement if any
    desire_ble_adv_stop();

    // Init state machine for EBID management
    _ebid_mgr_init(ebid, slice_adv_time_sec, ebid_adv_time_sec);

    // schedule advertisements
    event_timeout_set(&_update_timeout_evt, TICK_EVENT_INTERVAL);
}


void desire_ble_adv_stop(void)
{
    // notify stop event: cancel current advertisement ticker, reset
    event_timeout_clear(&_update_timeout_evt);
}

static void _tick_event_handler(event_t *e)
{
    (void)e;
    bool done;

    DEBUG("[Tick]\n");
    if (IS_ACTIVE(ENABLE_DEBUG)) {
        dbg_print_ebid_mgr();
    }

    ble_advertise_once(&(ebid_mgr.ble_adv_payload));

    if (_user_adv_cb) {
        _user_adv_cb(ztimer_now(ZTIMER_MSEC), NULL);
    }
    // if must change ebid, regenerate ebid
    done = _ebid_mgr_tick();

    // schedule next update event if advertisement duration not reached
    if (!done) {
        event_timeout_set(&_update_timeout_evt, TICK_EVENT_INTERVAL);
    }
}


/// EBID management module internals
static void dbg_print_ebid_mgr(void)
{
    DEBUG("Current Ebid Information:\n");
    dbg_dump_buffer("\t ebid = ", ebid_get(&ebid_mgr.ebid), EBID_SIZE, '\n');
    DEBUG("\t cid = %"PRIx32", sid=%d\n", ebid_mgr.cid, ebid_mgr.sid);
    dbg_dump_buffer("\t current_ebid_slice = ", ebid_mgr.current_ebid_slice,
                    EBID_SLICE_SIZE_LONG, '\n');

    DEBUG("Current BLE Adv Service Data Payload:\n");
    dbg_dump_buffer("\t ble_adv_payload = ", ebid_mgr.ble_adv_payload.bytes,
                    DESIRE_ADV_PAYLOAD_SIZE, '\n');

    DEBUG("Current timings\n");
    DEBUG("\t ticks: slice=%"PRIu32", ebid=%"PRIu32"", ebid_mgr.ticks.slice,
          ebid_mgr.ticks.ebid);
    DEBUG("\t limits: slice=%"PRIu16", ebid=%"PRIu16"\n", ebid_mgr.limits.slice_adv_time_sec,
          ebid_mgr.limits.ebid_adv_time_sec);

}


static void _ebid_mgr_init(ebid_t *ebid, uint16_t slice_adv_time_sec,
                           uint16_t ebid_adv_time_sec)
{
    // Generate a random CID that remains the same for the given EBID during the advertisement period
    ebid_mgr.cid = random_uint32() & MASK_CID;
    ebid_mgr.ebid = *ebid; // TODO a memcpy
    ebid_mgr.sid = 0;
    ebid_mgr.current_ebid_slice = ebid_get_slice1(&ebid_mgr.ebid);

    desire_ble_adv_payload_build(&ebid_mgr.ble_adv_payload, ebid_mgr.sid,
                                 ebid_mgr.cid, ebid_mgr.current_ebid_slice);

    ebid_mgr.limits.slice_adv_time_sec = slice_adv_time_sec;
    ebid_mgr.limits.ebid_adv_time_sec = ebid_adv_time_sec;
    ebid_mgr.ticks.slice = 0;
    ebid_mgr.ticks.ebid = 0;
}

static bool _ebid_mgr_tick(void)
{
    bool done = false;

    ebid_mgr.ticks.ebid++;
    ebid_mgr.ticks.slice++;
    uint8_t slice[EBID_SLICE_SIZE_LONG];
    memset(&slice, '\0', EBID_SLICE_SIZE_LONG);

    if (ebid_mgr.ticks.ebid < ebid_mgr.limits.ebid_adv_time_sec) {
        // EBID did not expire, check the slice
        if (ebid_mgr.ticks.slice >= ebid_mgr.limits.slice_adv_time_sec) {
            ebid_mgr.ticks.slice = 0;
            DEBUG(">>>> SLICE Renewal Event\n");
            // slice expired, switch to new slice and update metadata: current slice and adv payload (service data)
            ebid_mgr.sid = ((ebid_mgr.sid + 1) % 4);
            switch (ebid_mgr.sid) {
                case 0:
                    memcpy(slice, ebid_get_slice1(
                               &ebid_mgr.ebid), EBID_SLICE_SIZE_LONG);
                    break;
                case 1:
                    memcpy(slice, ebid_get_slice2(
                               &ebid_mgr.ebid), EBID_SLICE_SIZE_LONG);
                    break;
                case 2:
                    /* DESIRE sends sends the third slice with front padding so
                       ignore first 4 bytes:
                       https://gitlab.inria.fr/aboutet1/test-bluetooth/-/blob/master/app/src/main/java/fr/inria/desire/ble/models/AdvPayload.kt#L54
                     */
                    memcpy(slice + EBID_SLICE_SIZE_PAD,
                           ebid_get_slice3(
                               &ebid_mgr.ebid), EBID_SLICE_SIZE_SHORT);
                    break;
                case 3:
                    memcpy(slice, ebid_get_xor(
                               &ebid_mgr.ebid), EBID_SLICE_SIZE_LONG);
                    break;
                default:
                    assert(false); // Should not happen
                    break;
            }
            desire_ble_adv_payload_build(&ebid_mgr.ble_adv_payload,
                                         ebid_mgr.sid, ebid_mgr.cid,
                                         slice);
        }
    }
    else {
        // EBID expired, reset and regenerate EBID
        DEBUG(">>>> EBID Advetismend Ended\n");
        done = true;
    }

    return done;
}

// BLE Advetisement management: one shot advetise by settting adv duration to minimum
static struct  ble_gap_adv_params adv_params;
static void ble_advertise_once(desire_ble_adv_payload_t *adv_payload)
{
    int nimlble_ret;

    nimble_autoadv_reset();

    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_NON;
    nimble_autoadv_set_ble_gap_adv_params(&adv_params);
    nimble_auto_adv_set_adv_duration(BLE_NIMBLE_ADV_DURATION_MS);

    // Add tx power field
    int8_t phy_txpwr_dbm = ble_current_tx_pwr(); // FIXME use services from the ble stack to include this automatically avoiding calling the driver

    nimlble_ret = nimble_autoadv_add_field(BLE_GAP_AD_TX_POWER_LEVEL,
                                           &phy_txpwr_dbm,
                                           sizeof(phy_txpwr_dbm));
    assert(nimlble_ret == BLUETIL_AD_OK);

    // Add service data uuid
    uint16_t service_data = DESIRE_SERVICE_UUID16;

    nimlble_ret = nimble_autoadv_add_field(BLE_GAP_AD_UUID16_COMP,
                                           &service_data, sizeof(service_data));
    assert(nimlble_ret == BLUETIL_AD_OK);


    // Add service data field
    nimlble_ret = nimble_autoadv_add_field(BLE_GAP_AD_SERVICE_DATA_UUID16,
                                           adv_payload->bytes,
                                           DESIRE_ADV_PAYLOAD_SIZE);
    assert(nimlble_ret == BLUETIL_AD_OK);

    nimble_autoadv_start(); // start, this will end after 10 ms approx

    (void)nimlble_ret;
}

void desire_ble_adv_set_cb(ble_adv_cb_t cb)
{
    _user_adv_cb = cb;
}

uint32_t desire_ble_adv_get_cid(void)
{
    return ebid_mgr.cid;
}
