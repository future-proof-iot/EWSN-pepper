#include <stdio.h>
#include <stdlib.h>

#include "pepper.h"
#if IS_USED(MODULE_PEPPER_SRV)
#include "pepper_srv.h"
#endif

#include "msg.h"
#include "shell.h"
#include "shell_commands.h"

#define MAIN_QUEUE_SIZE     (4)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

int main(void)
{
    pepper_init();
#if IS_USED(MODULE_PEPPER_SRV)
    pepper_srv_init(CONFIG_PEPPER_LOW_EVENT_PRIO);
#endif
    /* the shell contains commands that receive packets via GNRC and thus
       needs a msg queue */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
