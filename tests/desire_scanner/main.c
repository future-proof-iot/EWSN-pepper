#include <stdio.h>
#include <stdlib.h>

#include "shell.h"
#include "shell_commands.h"

#include "desire_ble_scan.h"
#include "ble_pkt_dbg.h"

/* default scan duration (20s) */
#define DEFAULT_DURATION       (20 * US_PER_SEC)

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
    printf(">> Desire packet (service_uuid_16=0x%X): ts=%ld, rssi=%d, host = ", adv_payload->data.service_uuid_16, ts, rssi);
    dbg_print_addr(addr);
    putchar('\n');

    printf("\t sid = %d, cid=0x%lX, md_version=0x%lX\n", sid, cid, adv_payload->data.md_version);
    dbg_dump_buffer("\t ebid_slice = ", adv_payload->data.ebid_slice, EBID_SLICE_SIZE_LONG, '\n');
}

int _cmd_desire_scan(int argc, char **argv)
{
    uint32_t duration = DEFAULT_DURATION;

    if ((argc == 2) && (memcmp(argv[1], "help", 4) == 0)) {
        printf("usage: %s [duration in ms]\n", argv[0]);
        return 0;
    }
    if (argc >= 2) {
        duration = (uint32_t)(atoi(argv[1]) * 1000);
    }
    printf("Scanning for %ld\n", duration);
    desire_ble_scan(duration, detection_cb);

    return 0;
}

static const shell_command_t _commands[] = {
    { "scan", "trigger a BLE scan of Desire packets", _cmd_desire_scan },
    { NULL, NULL, NULL }
};

int main(void)
{
    puts("Desire BLE Scanner Test Application");

    /* initialize the desire scanner */
    desire_ble_scan_init();

    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
