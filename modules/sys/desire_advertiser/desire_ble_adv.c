#include "desire_ble_adv.h"

#include "event.h"
#include "event/timeout.h"
#include "event/thread.h"

#include "random.h"
#include "assert.h"

#include "net/bluetil/ad.h"
#include "nimble/hci_common.h"

#include "nimble_autoadv.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"


// BLE Advetisement
#define BLE_NIMBLE_ADV_DURATION_MS 10
static void ble_advertise_once(desire_ble_adv_payload_t *adv_payload);

// Ticker Event thread
#define TICK_EVENT_INTERVAL (1 * US_PER_SEC)
static event_queue_t _eq;
static event_t _update_evt;
static event_timeout_t _update_timeout_evt;
static void _tick_event_handler(event_t *e);
static char event_thread_stack[THREAD_STACKSIZE_MAIN];

// EBID generation and slicing management
typedef struct {
    /* Crypto key pairs */
    crypto_manager_keys_t keys;
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
static void _ebid_generate(ebid_t *ebid, crypto_manager_keys_t *keys,
                           uint32_t *cid);
static void _ebid_mgr_init(uint16_t slice_adv_time_sec,
                           uint16_t ebid_adv_time_sec);
static void _ebid_mgr_tick(void);

void desire_ble_adv_init(void)
{
    // init ebid if static mode, generate ebid once on init
    if (DESIRE_STATIC_EBID == 1) {
        _ebid_generate(&ebid_mgr.ebid, &ebid_mgr.keys, &ebid_mgr.cid);
    }

    // init event loop ticker thread
    // create a thread that runs the event loop: event_thread_init
    event_queue_init(&_eq);
    _update_evt.handler = _tick_event_handler;
    event_timeout_init(&_update_timeout_evt, &_eq, &_update_evt);

    // Thread that will run an event loop (event_loop) for handling TICK_EVENT_INTERVAL second tick
    event_thread_init(&_eq, event_thread_stack, sizeof(event_thread_stack),
                      EVENT_QUEUE_PRIO_MEDIUM);
}

void desire_ble_adv_start(uint16_t slice_adv_time_sec,
                          uint16_t ebid_adv_time_sec)
{
    desire_ble_adv_stop();
    _ebid_mgr_init(slice_adv_time_sec, ebid_adv_time_sec);
    event_timeout_set(&_update_timeout_evt, TICK_EVENT_INTERVAL);
}


void desire_ble_adv_stop(void)
{
    // notify stop event: cancel current advertisement ticker, reset
    event_timeout_clear(&_update_timeout_evt);
}

static void print_ebid_mgr(void);
static void _tick_event_handler(event_t *e)
{
    (void)e;

    puts("[Tick]");
    print_ebid_mgr();

    ble_advertise_once(&(ebid_mgr.ble_adv_payload));

    // if must change ebid, regenerate ebid
    _ebid_mgr_tick();

    // schedule next update event
    event_timeout_set(&_update_timeout_evt, TICK_EVENT_INTERVAL);
}


/// EBID management module internals
static inline void dump_buffer(const char *prefix, uint8_t *buf, size_t size,
                               char suffix)
{
    printf("%s", prefix);
    for (unsigned int i = 0; i < size; i++) {
        printf("%.2X", buf[i]);
        putchar((i < (size - 1))?':':suffix);
    }
}
static void print_ebid_mgr(void)
{
    printf("Current Ebid Information:\n");
    dump_buffer("\t ebid = ", ebid_get(&ebid_mgr.ebid), EBID_SIZE, '\n');
    printf("\t cid = %lX, sid=%d\n", ebid_mgr.cid, ebid_mgr.sid);
    dump_buffer("\t current_ebid_slice = ", ebid_mgr.current_ebid_slice,
                EBID_SLICE_SIZE_LONG, '\n');

    printf("Current BLE Adv Service Data Payload:\n");
    dump_buffer("\t ble_adv_payload = ", ebid_mgr.ble_adv_payload.bytes,
                DESIRE_ADV_PAYLOAD_SIZE, '\n');

    printf("Current timings\n");
    printf("\t ticks: slice=%ld, ebid=%ld", ebid_mgr.ticks.slice,
           ebid_mgr.ticks.ebid);
    printf("\t limits: slice=%d, ebid=%d\n", ebid_mgr.limits.slice_adv_time_sec,
           ebid_mgr.limits.ebid_adv_time_sec);

}

static void _ebid_generate(ebid_t *ebid, crypto_manager_keys_t *keys,
                           uint32_t *cid)
{
    int ret;

    ebid_init(ebid);

    ret = crypto_manager_gen_keypair(keys);
    assert(ret == 0);

    ret = ebid_generate(ebid, keys);
    assert(ret == 0);

    *cid = random_uint32() & MASK_CID;

    (void)ret;
}

static void _ebid_mgr_init(uint16_t slice_adv_time_sec,
                           uint16_t ebid_adv_time_sec)
{
    // if not static i.e dynamic, regenerate ebid and a cid
    // Note: crypto_manager_gen_keypair is called, this may not be required (only at init?)
    if (DESIRE_STATIC_EBID == 0) {
        _ebid_generate(&ebid_mgr.ebid, &ebid_mgr.keys, &ebid_mgr.cid);
    }

    ebid_mgr.sid = 0;
    ebid_mgr.current_ebid_slice = ebid_get_slice1(&ebid_mgr.ebid);

    desire_ble_adv_payload_build(&ebid_mgr.ble_adv_payload, ebid_mgr.sid,
                                 ebid_mgr.cid, ebid_mgr.current_ebid_slice);

    ebid_mgr.limits.slice_adv_time_sec = slice_adv_time_sec;
    ebid_mgr.limits.ebid_adv_time_sec = ebid_adv_time_sec;
    ebid_mgr.ticks.slice = 0;
    ebid_mgr.ticks.ebid = 0;
}

static void _ebid_mgr_tick(void)
{
    ebid_mgr.ticks.ebid++;
    ebid_mgr.ticks.slice++;

    if (ebid_mgr.ticks.ebid < ebid_mgr.limits.ebid_adv_time_sec) {
        // EBID did not expire, check the slice
        if (ebid_mgr.ticks.slice >= ebid_mgr.limits.slice_adv_time_sec) {
            ebid_mgr.ticks.slice = 0;
            puts(">>>> SLICE Renewal Event");
            // slice expired, switch to new slice and update metadata: current slice and adv payload (service data)
            ebid_mgr.sid = ((ebid_mgr.sid + 1) % 4);
            switch (ebid_mgr.sid) {
            case 0:
                ebid_mgr.current_ebid_slice = ebid_get_slice1(&ebid_mgr.ebid);
                break;
            case 1:
                ebid_mgr.current_ebid_slice = ebid_get_slice2(&ebid_mgr.ebid);
                break;
            case 2:
                ebid_mgr.current_ebid_slice = ebid_get_slice3(&ebid_mgr.ebid);
                break;
            case 3:
                ebid_mgr.current_ebid_slice = ebid_get_xor(&ebid_mgr.ebid);
                break;
            default:
                assert(false); // Should not happen
                break;
            }
            desire_ble_adv_payload_build(&ebid_mgr.ble_adv_payload,
                                         ebid_mgr.sid, ebid_mgr.cid,
                                         ebid_mgr.current_ebid_slice);
        }
    }
    else {
        // EBID expired, reset and regenerate EBID
        // TODO: check if key pairs shoud be regenerated on each EPOCH??, if not
        puts(">>>> EBID Renewal Event");
        _ebid_mgr_init(ebid_mgr.limits.slice_adv_time_sec,
                       ebid_mgr.limits.ebid_adv_time_sec);
    }
}

// BLE Advetisement management: one shot advetise by settting adv duration to minimum
static struct  ble_gap_adv_params adv_params;
static void ble_advertise_once(desire_ble_adv_payload_t *adv_payload)
{
    int nimlble_ret;

    nimble_autoadv_init();

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
    uint16_t service_data = 0x6666;

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
