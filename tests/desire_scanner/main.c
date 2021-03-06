#include <stdio.h>
#include <stdlib.h>

#include "timex.h"
#include "shell.h"
#include "shell_commands.h"

#include "ble_scanner_params.h"
#include "desire_ble_scan.h"
#include "ble_pkt_dbg.h"

/* default scan duration (20s) */
#define DEFAULT_DURATION_MS       (20 * MS_PER_SEC)

static struct {
    uint32_t cid;
    ebid_t ebid;
} ebid_tracker;

static void init_ebid_tracker(uint32_t cid)
{
    ebid_init(&ebid_tracker.ebid);
    ebid_tracker.cid = cid;
}

/* Dummy detection callback */
void detection_cb(uint32_t ts,
                  const ble_addr_t *addr, int8_t rssi,
                  const desire_ble_adv_payload_t *adv_payload)
{
    // decode header
    uint32_t cid;
    uint8_t sid;
    decode_sid_cid(adv_payload->data.sid_cid, &sid, &cid);

    // dump
    printf(">> Desire packet (service_uuid_16=0x%X): ts=%"PRIu32", rssi=%d, ", adv_payload->data.service_uuid_16, ts, rssi);
    dbg_print_ble_addr(addr);
    printf("\t sid = %d, cid=0x%lX, md_version=0x%02x\n", sid, cid, adv_payload->data.md_version);
    dbg_dump_buffer("\t ebid_slice = ", adv_payload->data.ebid_slice, EBID_SLICE_SIZE_LONG, '\n');

    // Reconstruct ebid when possible
    // if new cid, reset else update slice
    if ( ebid_tracker.cid != cid) {
        puts(">>>>>>>>>>EBID RESET");
        init_ebid_tracker(cid);
    }
    // TODO assert sid range
    if(sid != 2) {
        ebid_set_slice(&ebid_tracker.ebid, adv_payload->data.ebid_slice, sid);
    } else {
        // FIXME Desire sets padding bytes first !!
        /* DESIRE sends sends the third slice with front padding so
            ignore first 4 bytes:
            https://gitlab.inria.fr/aboutet1/test-bluetooth/-/blob/master/app/src/main/java/fr/inria/desire/ble/models/AdvPayload.kt#L54
        */
        ebid_set_slice(&ebid_tracker.ebid, adv_payload->data.ebid_slice+EBID_SLICE_SIZE_PAD, sid);
    }


    int rc = ebid_reconstruct(&ebid_tracker.ebid);
    printf("EBID Reconstruct status = %d\n", rc);
    if (!rc) {
        puts(">>>>>>>>>>EBID RECONSTRUCTED");
        ebid_t* ebid = &ebid_tracker.ebid;
        dbg_dump_buffer("Reconstructed ebid = ", ebid_get(ebid), EBID_SIZE, '\n');
        dbg_dump_buffer("\t slice_1 = ", ebid_get_slice1(ebid), EBID_SLICE_SIZE_LONG, '\n');
        dbg_dump_buffer("\t slice_2 = ", ebid_get_slice2(ebid), EBID_SLICE_SIZE_LONG, '\n');
        dbg_dump_buffer("\t slice_3 = ", ebid_get_slice3(ebid), EBID_SLICE_SIZE_LONG, '\n');
        dbg_dump_buffer("\t slice_xor = ", ebid_get_xor(ebid), EBID_SLICE_SIZE_LONG, '\n');
    }
}

int _cmd_desire_scan(int argc, char **argv)
{
    uint32_t duration = DEFAULT_DURATION_MS;

    if ((argc == 2) && (memcmp(argv[1], "help", 4) == 0)) {
        printf("usage: %s [duration in ms]\n", argv[0]);
        return 0;
    }
    if (argc >= 2) {
        duration = (uint32_t)(atoi(argv[1]));
    }
    printf("Scanning for %"PRIu32"\n", duration);
    ble_scan_params_t params = CONFIG_BLE_LOW_LATENCY_PARAMS;
    desire_ble_scan_start(&params, duration);

    return 0;
}

int _cmd_desire_stop(int argc, char **argv)
{
    desire_ble_scan_stop();

    (void)argc;
    (void)argv;
    return 0;
}
static const shell_command_t _commands[] = {
    { "scan", "trigger a BLE scan of Desire packets", _cmd_desire_scan },
    { "stops", "stops scanning of Desire packets", _cmd_desire_stop },
    { NULL, NULL, NULL }
};

int main(void)
{
    puts("Desire BLE Scanner Test Application");

    /* initialize the desire scanner */
    desire_ble_scan_init(detection_cb);
    init_ebid_tracker(0);

    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
