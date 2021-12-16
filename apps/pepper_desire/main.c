#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include "shell.h"
#include "shell_commands.h"
#include "pepper.h"

#define MAIN_QUEUE_SIZE     (4)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

static const shell_command_t _commands[] = {
    { NULL, NULL, NULL }
};

int main(void)
{
    pepper_init();
    /* the shell contains commands that receive packets via GNRC and thus
       needs a msg queue */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
