
#include <stdio.h>

#include "desire_ble_adv.h"


int main(void)
{
    puts("Init Desire Adv module");
    
    desire_ble_adv_init();
    
    puts("Init Desire Adv module OK");

    puts("Starting Desire Adv module");
    
    //desire_ble_adv_start(2, 1*SEC_PER_MIN);
    desire_ble_adv_start(DESIRE_DEFAULT_SLICE_ROTATION_PERIOD_SEC, DESIRE_DEFAULT_EBID_ROTATION_PERIOD_SEC);
    
    puts("Starting Desire Adv module OK");

    return 0;
}
