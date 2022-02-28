#include <stdio.h>
#include <stdlib.h>

#include "kernel_defines.h"
#include "msg.h"
#include "shell.h"
#include "shell_commands.h"
#include "pepper.h"

#if IS_USED(MODULE_UWB_DW1000)
#include "uwb_dw1000.h"
#include "uwb_dw1000_params.h"

static dw1000_dev_instance_t dev;
#endif

#define MAIN_QUEUE_SIZE     (4)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

int main(void)
{

#if IS_USED(MODULE_UWB_DW1000)
    /* inits the dw1000 devices linked list */
    uwb_dw1000_init();
    /* setup dw1000 device */
    uwb_dw1000_setup(&dev, (void *) &dw1000_params[0]);
    /* this will start a thread handling dw1000 device */
    uwb_dw1000_config_and_start(&dev);
    dw1000_dev_configure_sleep(&dev);
    dw1000_dev_enter_sleep(&dev);
#endif

    /* init pepper */
    pepper_init();
    /* start pepper with default parameters */
    pepper_start_params_t params = {
        .epoch_duration_s = CONFIG_EPOCH_DURATION_SEC,
        .epoch_iterations = 0,
        .adv_itvl_ms = CONFIG_BLE_ADV_ITVL_MS,
        .advs_per_slice = CONFIG_ADV_PER_SLICE,
        .scan_itvl_ms = CONFIG_BLE_SCAN_ITVL_MS,
        .scan_win_ms = CONFIG_BLE_SCAN_WIN_MS,
        .align = false,
    };
    pepper_start(&params);

    /* the shell contains commands that receive packets via GNRC and thus
       needs a msg queue */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
    return 0;
}
