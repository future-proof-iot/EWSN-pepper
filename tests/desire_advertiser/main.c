#include <stdio.h>
#include <stdlib.h>

#include "shell.h"
#include "shell_commands.h"

#include "desire_ble_adv.h"

static void _adv_cb(uint32_t advs, void *arg)
{
    (void)arg;
    (void)advs;
    desire_ble_adv_status_print();
}

int main(void)
{
    puts("Desire BLE Advertiser Test Application");

    /* initialize the static ebid and desire advertiser */
    desire_ble_adv_set_cb(_adv_cb);
    desire_ble_adv_init_threaded();

    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
