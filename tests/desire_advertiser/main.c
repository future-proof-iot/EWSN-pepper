#include <stdio.h>
#include <stdlib.h>

#include "shell.h"
#include "shell_commands.h"

#include "event/thread.h"
#include "desire_ble_adv.h"
#include "ble_pkt_dbg.h"

static const uint8_t desire_ebid_slice_1[EBID_SLICE_SIZE_LONG] = {
    0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x20, 0x2e
};

static const uint8_t desire_ebid_slice_2[EBID_SLICE_SIZE_LONG] = {
    0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x20, 0xfe
};

static const uint8_t desire_ebid_slice_3[EBID_SLICE_SIZE_SHORT] = {
    0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17
};
/*
static const uint8_t desire_ebid[EBID_SIZE] = {
    0x20, 0x21, 0x22, 0x23, 0x24,
    0x25, 0x26, 0x27, 0x28, 0x29,
    0x20, 0x2e, 0x10, 0x11, 0x12,
    0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x20, 0xfe, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15,
    0x16, 0x17
};
*/
static const uint8_t desire_ebid_xor[EBID_SLICE_SIZE_LONG] = {
    0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27,
    0x30, 0x30, 0x00, 0xd0
};

/* Current EBID */
static ebid_t ebid;

static void generate_ebid(ebid_t* ebid)
{
    // static ebid
    ebid_init(ebid);

    ebid_set_slice1(ebid, desire_ebid_slice_1);
    ebid_set_slice2(ebid, desire_ebid_slice_2);
    ebid_set_slice3(ebid, desire_ebid_slice_3);
    ebid_set_xor(ebid, desire_ebid_xor);
}

static void print_ebid(ebid_t* ebid) {
    dbg_dump_buffer("Current ebid [STATIC] = ", ebid_get(ebid), EBID_SIZE, '\n');
    dbg_dump_buffer("\t slice_1 = ", ebid_get_slice1(ebid), EBID_SLICE_SIZE_LONG, '\n');
    dbg_dump_buffer("\t slice_2 = ", ebid_get_slice2(ebid), EBID_SLICE_SIZE_LONG, '\n');
    dbg_dump_buffer("\t slice_3 = ", ebid_get_slice3(ebid), EBID_SLICE_SIZE_LONG, '\n');
    dbg_dump_buffer("\t slice_xor = ", ebid_get_xor(ebid), EBID_SLICE_SIZE_LONG, '\n');
}

/* CLI handlers */

int _cmd_desire_adv_start(int argc, char **argv)
{
    uint16_t slice_adv_time_sec = CONFIG_SLICE_ROTATION_T_S;
    uint16_t ebid_adv_time_sec = CONFIG_EBID_ROTATION_T_S;

    if ((argc == 2) && (memcmp(argv[1], "help", 4) == 0)) {
        printf("usage: %s <slice advertisment duration in sec> <ebid advertisment duration in minutes>\n", argv[0]);
        printf("default: %s %d %ld", argv[0], CONFIG_SLICE_ROTATION_T_S, CONFIG_EBID_ROTATION_T_S/60);
        return 0;
    }
    if (argc >= 3) {
        slice_adv_time_sec = (uint32_t)(atoi(argv[1]));
        ebid_adv_time_sec = (uint32_t)(atoi(argv[2])*60);
    }
    printf("Advetising with:  slice_adv_time_sec = %d,  ebid_adv_time_sec = %d\n", slice_adv_time_sec, ebid_adv_time_sec);
    desire_ble_adv_start(&ebid, slice_adv_time_sec, ebid_adv_time_sec);

    return 0;
}

int _cmd_desire_adv_stop(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    desire_ble_adv_stop();
    printf("Stopped ongoing advertisements (if any)\n");

    return 0;
}

int _cmd_desire_get_ebid(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    print_ebid(&ebid);

    return 0;
}

static const shell_command_t _commands[] = {
    { "adv_start", "trigger a BLE advertisment of Desire packets", _cmd_desire_adv_start },
    { "adv_stop", "trigger a BLE advertisment of Desire packets", _cmd_desire_adv_stop },
    { "ebid", "trigger a BLE advertisment of Desire packets", _cmd_desire_get_ebid},
    { NULL, NULL, NULL }
};

int main(void)
{
    puts("Desire BLE Advertiser Test Application");

    /* initialize the static ebid and desire advertiser */

    generate_ebid(&ebid);
    desire_ble_adv_init(EVENT_PRIO_HIGHEST);

    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
