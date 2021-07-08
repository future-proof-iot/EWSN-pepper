#include <stdio.h>
#include <stdlib.h>

#include "shell.h"
#include "shell_commands.h"

#include "desire_coap_client.h"


/* CLI handlers */

int _cmd_post_ertl(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    puts("TBD");

    return 0;
}

int _cmd_post_infected(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    puts("TBD");

    return 0;
}

int _cmd_get_esr(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    puts("TBD");

    return 0;
}

int _cmd_coap_srv(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    puts("TBD");

    return 0;
}

int _cmd_uid(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    
    printf("\t uid set to %s \n", DESIRE_COAP_DEVICE_UID);

    


    return 0;
}

static const shell_command_t _commands[] = {
    { "ertl", "POST a static ertl payload", _cmd_post_ertl },
    { "infected", "POST infection status", _cmd_post_infected },
    { "esr", "GET exposure boolean ", _cmd_get_esr},
    { "server", "get/sets the coap server host and port", _cmd_coap_srv },
    { "uid", "gets the device 16-bit uid", _cmd_uid},
    { NULL, NULL, NULL}
};

int main(void)
{
    puts("Desire COAP Client Test Application");

    /* initialize the coap client */
    desire_coap_client_init();

    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
