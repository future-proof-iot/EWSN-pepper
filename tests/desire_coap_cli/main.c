#include <stdio.h>
#include <stdlib.h>

#include "shell.h"
#include "shell_commands.h"

#include "state_manager.h"
#include "coap/utils.h"
#include "net/coap.h"
#include "net/sock/udp.h"

#include "nanocbor/nanocbor.h"
#include "msg.h"

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
#include "desire_ble_scan.h"
#include "desire_ble_scan_params.h"

/* default scan duration (20s) */
#define DEFAULT_DURATION_MS       (20 * MS_PER_SEC)
#endif

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

/* dummy ertl cbor data */
static uint8_t ertl[] = {
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

static bool infected = false;

static sock_udp_ep_t remote;
static coap_block_ctx_t block_ctx;
static coap_req_ctx_t get_ctx;

void _block_end_callback(int ret, void *data, size_t len, void *arg)
{
    (void)data;
    (void)len;
    (void)arg;
    printf("block transactions finished ret=(%d)\n", ret);
}

int _cmd_post_ertl(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    coap_req_ctx_init(&block_ctx.req_ctx, _block_end_callback, NULL);
    coap_block_post(&remote, &block_ctx, ertl, sizeof(ertl), "/DW0456/ertl",
                    COAP_FORMAT_CBOR, COAP_TYPE_NON);
    return 0;
}

int _cmd_set_infected(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s [y,n]\n", argv[0]);
        return -1;
    }

    if (*argv[1] == 'y') {
        infected = true;
        return 0;
    }
    else if (*argv[1] == 'n') {
        infected = false;
        return 0;
    }
    else {
        puts("invalid option");
        return -2;
    }
}

int _cmd_post_infected(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    uint8_t buf[8];

    nanocbor_encoder_t enc;
    nanocbor_encoder_init(&enc, buf, sizeof(buf));
    nanocbor_fmt_tag(&enc, 0xCAFA);
    nanocbor_fmt_array(&enc, 1);
    nanocbor_fmt_bool(&enc, infected);
    size_t len = nanocbor_encoded_len(&enc);

    coap_post(&remote, NULL, buf, len, "/DW0456/infected",
              COAP_FORMAT_CBOR, COAP_TYPE_CON);

    return 0;
}

void _get_callback(int ret, void *data, size_t len, void *arg)
{
    (void)ret;
    (void)arg;
    bool status = state_manager_get_esr();
    state_manager_esr_load_cbor(data, len);
    printf("state_old=(%d), state_new=(%d)\n", status, state_manager_get_esr());
}

int _cmd_get_esr(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    coap_req_ctx_init(&get_ctx, _get_callback, NULL);
    coap_get(&remote, &get_ctx, "/DW0456/esr", COAP_FORMAT_CBOR, COAP_TYPE_NON);

    return 0;
}

int _cmd_coap_srv(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage: %s ip_addr port\n", argv[0]);
        return -1;
    }

    coap_init_remote(&remote, argv[1], atoi(argv[2]));
    return 0;
}

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
/* Dummy detection callback */
void detection_cb(uint32_t ts,
                  const ble_addr_t *addr, int8_t rssi,
                  const desire_ble_adv_payload_t *adv_payload)
{
    (void)ts;
    (void)addr;
    (void)rssi;
    (void)adv_payload;
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
    printf("Scanning for %" PRIu32 "\n", duration);
    desire_ble_scan_start(duration);

    return 0;
}

int _cmd_desire_stop(int argc, char **argv)
{
    desire_ble_scan_stop();

    (void)argc;
    (void)argv;
    return 0;
}
#endif

static const shell_command_t _commands[] = {
    { "ertl", "POST a static ertl payload", _cmd_post_ertl },
    { "infected", "POST infection status", _cmd_post_infected },
    { "status", "sets infection status", _cmd_set_infected },
    { "esr", "GET exposure boolean ", _cmd_get_esr },
    { "server", "get/sets the coap server host and port", _cmd_coap_srv },
#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
    { "scan", "trigger a BLE scan of Desire packets", _cmd_desire_scan },
    { "stops", "stops scanning of Desire packets", _cmd_desire_stop },
#endif
    { NULL, NULL, NULL }
};

/* dummy define to get rid of editor higliting */
#ifndef BOARD_NATIVE
#define BOARD_NATIVE    0
#endif

int main(void)
{
    puts("Desire COAP Client Test Application");

#if IS_USED(MODULE_DESIRE_SCANNER_NETIF)
    desire_ble_scan_init(&desire_ble_scanner_params, detection_cb);
#endif

    if (BOARD_NATIVE) {
        ipv6_addr_t addr;
        const char addr_str[] = "2001:db8::2";
        ipv6_addr_from_str(&addr, addr_str);
        gnrc_netif_ipv6_addr_add(gnrc_netif_iter(NULL), &addr, 64, 0);
        coap_init_remote(&remote, "2001:db8::1", 5683);
    }
    else {
        coap_init_remote(&remote, "fd00:dead:beef::1", 5683);
    }
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    /* initialize the coap client */

    /* init state manager */
    state_manager_init();
    state_manager_set_esr(true);

    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
