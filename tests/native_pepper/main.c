#include <stdio.h>
#include <stdlib.h>

#include "shell.h"
#include "shell_commands.h"

#include "state_manager.h"
#include "uwb_epoch.h"
#include "bpf/uwb_ed_shared.h"
#include "coap/utils.h"

#include "timex.h"
#include "ztimer.h"
#include "random.h"
#include "net/coap.h"
#include "net/sock/udp.h"
#include "msg.h"
#include "event/thread.h"
#include "event/periodic.h"
#include "event/callback.h"

/* TODO: move this to a common header for all PEPPER apps */
#ifndef CONFIG_EBID_ROTATION_T_S
#define CONFIG_EBID_ROTATION_T_S    (15 * SEC_PER_MIN)
#endif

#define MAIN_QUEUE_SIZE             (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
static sock_udp_ep_t remote;

/* event for the end of EPOCH */
static event_periodic_t _epoch_end;

static void _random_uwb_contact(uwb_contact_data_t *data)
{
    random_bytes(data->pet.et, PET_SIZE);
    random_bytes(data->pet.rt, PET_SIZE);
    data->avg_d_cm = random_uint32_range(0, MAX_DISTANCE_CM);
    data->req_count = random_uint32_range(10, 100);
    data->exposure_s = random_uint32_range(MIN_EXPOSURE_TIME_S,
                                           CONFIG_EBID_ROTATION_T_S);
}

static void _random_uwb_epoch(uwb_epoch_data_t *data)
{
    memset(data, '\0', sizeof(uwb_epoch_data_t));
    data->timestamp = (uint16_t)ztimer_now(ZTIMER_SEC);
    for (uint8_t i = 0; i < CONFIG_EPOCH_MAX_ENCOUNTERS; i++) {
        _random_uwb_contact(&data->contacts[i]);
    }
}

static void _serialize_event_handler(event_t *event)
{
    (void)event;
    uwb_epoch_data_t data;

    _random_uwb_epoch(&data);
    printf("[pepper]: new uwb_epoch t=%" PRIu64 "\n",
           ztimer_now(ZTIMER_SEC));
    if (uwb_epoch_contacts(&data)) {
        state_manager_coap_send_ertl(&remote, &data);
    }
    uwb_epoch_serialize_printf(&data);

    printf("[pepper]: fetch exposure status\n");
    state_manager_coap_get_esr(&remote);
}

static event_t _serialize_event =
{ .handler = _serialize_event_handler };

static int _cmd_remote(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage: %s ip_addr port\n", argv[0]);
        return -1;
    }

    coap_init_remote(&remote, argv[1], atoi(argv[2]));
    return 0;
}

static int _cmd_uwb_rnd(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    uwb_epoch_data_t data;
    _random_uwb_epoch(&data);
    uwb_epoch_serialize_printf(&data);
    return 0;
}

int _cmd_post_infected(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s [y,n]\n", argv[0]);
        return -1;
    }

    if (*argv[1] == 'y') {
        printf("[pepper]: COVID positive!\n");
        state_manager_set_infected_status(true);
    }
    else if (*argv[1] == 'n') {
        state_manager_set_infected_status(false);
    }
    else {
        puts("invalid option");
        return -2;
    }

    state_manager_coap_send_infected(&remote);
    return 0;
}

static int _cmd_id(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("dwm1001 id: DW%s\n", state_manager_get_id());
    return 0;
}

int _cmd_post_ertl(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    uwb_epoch_data_t data;
    _random_uwb_epoch(&data);
    uwb_epoch_serialize_printf(&data);
    state_manager_coap_send_ertl(&remote, &data);

    return 0;
}

static const shell_command_t _commands[] = {
    { "ertl", "POST a static ertl payload", _cmd_post_ertl },
    { "remote", "get/sets remote endpoint", _cmd_remote },
    { "infected", "POST infection status", _cmd_post_infected },
    { "uwb-rnd", "generate and serialize a random set uf uwb data", _cmd_uwb_rnd },
    { "id", "Print device id", _cmd_id },
    { NULL, NULL, NULL }
};

int main(void)
{
    /* Initialize state manager */
    state_manager_init();
    printf("Pepper Native Mock, id: DW%s\n", state_manager_get_id());

    /* Set a default routable address for the native node */
    ipv6_addr_t addr;
    const char addr_str[] = "2001:db8::2";
    ipv6_addr_from_str(&addr, addr_str);
    gnrc_netif_ipv6_addr_add(gnrc_netif_iter(NULL), &addr, 64, 0);
    coap_init_remote(&remote, "2001:db8::1", 5683);

    /* Set up the message queue */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    /* setup end of uwb_epoch timeout event */
    (void)_serialize_event;
    (void)_epoch_end;
    event_periodic_init(&_epoch_end, ZTIMER_SEC, EVENT_PRIO_HIGHEST,
                        &_serialize_event);
    event_periodic_start(&_epoch_end, CONFIG_EBID_ROTATION_T_S);

    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
}
