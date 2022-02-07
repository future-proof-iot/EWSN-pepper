/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_pepper_srv
 * @{
 *
 * @file
 * @brief       Shell-based local server implementation. Simulate a remote server using a CLI.
 *
 * @author      Roudy Dagher <roudy.dagher@inria.fr>
 *
 * @}
 */
#include "pepper_srv.h"
#include "pepper.h"

#include "shell_commands.h"
#include "strings.h"

XFA_USE_CONST(pepper_srv_endpoint_t, pepper_srv_endpoints);

static struct {
    bool exposed;
    bool infected;
    epoch_data_t *epoch_data; // TODO make a thread safe copy
} _shell_state = { false, false, NULL };


int _shell_srv_init(event_queue_t *evt_queue)
{
    evt_queue = evt_queue;
    printf(">--< End point Shell : init !\n");
    return 0;
}


int _shell_srv_notify_epoch_data(epoch_data_t *epoch_data)
{
    _shell_state.epoch_data = epoch_data; // TODO make a thread safe copy

    printf(">--< End point shell : notify epoch_data, contacts = %d, ts = %ld\n",
           epoch_contacts(epoch_data), epoch_data->timestamp);

    return 0;
}

int _shell_srv_notify_infection(bool infected)
{
    _shell_state.infected = infected;
    // print infection data in json
    printf(">--< End point shell : infected ! %d\n", infected);

    return 0;
}


int _shell_srv_request_exposure(bool *esr /*out*/)
{
    *esr = _shell_state.exposed;

    return 0;
}

XFA_CONST(pepper_srv_endpoints, 0) pepper_srv_endpoint_t _pepper_srv_shell = {
    .init = _shell_srv_init,
    .notify_epoch_data = _shell_srv_notify_epoch_data,
    .notify_infection = _shell_srv_notify_infection,
    .request_exposure = _shell_srv_request_exposure
};

// SHELL endpoint cli for simulating Pepper (desire) remote server behaviour

static void _shell_pepperd_print_usage(void)
{
    puts("Usage:");
    puts(
        "\tpepperd exp [true|false] : sets/unsets flag indicating that his node is a contact case. If without params, dumps current exposure status");
    puts("\tpepperd inf : returns the infection flag indicating that his node is infected.");
    puts("\tpepperd ed : dumps last received epoch data");
}

static void _shell_pepperd_print_epoch_data(void)
{
    contact_data_serialize_all_printf(_shell_state.epoch_data, pepper_get_serializer_bn());
}

static void _shell_pepperd_print_infection(void)
{
    printf("Shell endoint : infected = %d\n", _shell_state.infected);
}

static void _shell_pepperd_print_exposure(void)
{
    printf("Shell endoint : exposed = %d\n", _shell_state.exposed);
}


static int _pepperd_shell_handler(int argc, char **argv)
{
    // parse
    if (argc < 2) {
        _shell_pepperd_print_usage();
        return -1;
    }

    // process
    if (!strcmp(argv[1], "exp")) {
        if (argc >= 2) {
            _shell_state.exposed = !strcasecmp(argv[2], "true");
        }
        _shell_pepperd_print_exposure();
        return 0;
    }

    if (!strcmp(argv[1], "inf")) {
        _shell_pepperd_print_infection();
        return 0;
    }

    if (!strcmp(argv[1], "ed")) {
        _shell_pepperd_print_epoch_data();
        return 0;
    }

    _shell_pepperd_print_usage();
    return -1;
}

SHELL_COMMAND(pepperd, "Pepper Server interface (Shell mode)", _pepperd_shell_handler);
