/*
 * Copyright (c) 2021 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Minimal bpf example
 *
 * @author      Koen Zandberg <koen.zandberg@inria.fr>
 *
 * @}
 */
#include <stdio.h>
#include <string.h>

#include "kernel_defines.h"
#include "msg.h"
#include "shell.h"
#include "net/gnrc/netif.h"
#include "net/ipv6/addr.h"
#include "net/nanocoap.h"
#include "suit/transport/coap.h"

#include "uwb_ed.h"

/* must be sorted by path (ASCII order) */
const coap_resource_t coap_resources[] = {
    COAP_WELL_KNOWN_CORE_DEFAULT_HANDLER,
    /* this line adds the whole "/suit"-subtree */
    SUIT_COAP_SUBTREE,
};
const unsigned coap_resources_numof = ARRAY_SIZE(coap_resources);

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

static int bpf_run(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    uwb_ed_t ed;
    ed.cumulative_d_cm = MAX_DISTANCE_CM * 4;
    ed.seen_first_s = 0;
    ed.seen_last_s = MIN_EXPOSURE_TIME_S + 1;
    ed.ebid.status.status = EBID_HAS_ALL;
    ed.req_count = 4;
    bool valid = uwb_ed_finish_bpf(&ed);
    printf("uwb_ed_finish_bpf: d=(%"PRIu32"), exposure=(%"PRIu32"), valid=(%d)\n",
        ed.cumulative_d_cm, uwb_ed_exposure_time(&ed), valid);
    return 0;
}

static const shell_command_t shell_commands[] = {
    { "run", "Calls the uwb_ed_finish_bpf function", bpf_run },
    { NULL, NULL, NULL }
};

/* dummy define to get rid of editor higliting */
#ifndef BOARD_NATIVE
#define BOARD_NATIVE    0
#endif

int main(void)
{
    /* the shell contains commands that receive packets via GNRC and thus
       needs a msg queue */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    if (BOARD_NATIVE) {
        ipv6_addr_t addr;
        const char addr_str[] = "2001:db8::2";
        ipv6_addr_from_str(&addr, addr_str);
        gnrc_netif_ipv6_addr_add(gnrc_netif_iter(NULL), &addr, 64, 0);
    }

    /* init bpf */
    uwb_ed_bpf_init();

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run_once(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
