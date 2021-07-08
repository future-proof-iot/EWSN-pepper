#include "desire_coap_client.h"

#include "fmt.h"
#include "luid.h"
#include "net/ieee802154.h"

#include "net/gnrc/netif.h"

#include <stdio.h>



/* Static Device UID guessed from the BLE mac address */
static char _DESIRE_COAP_DEVICE_UID[2*DESIRE_COAP_DEVICE_UID_BYTES+1];
const char* DESIRE_COAP_DEVICE_UID = &_DESIRE_COAP_DEVICE_UID[0];

/* Current coap server endpoint */
static uint16_t coap_srv_port;
static 



void desire_coap_cli_init(void) {
    /* Guess uid from BLE mac address, as the first iface */

    memset(_DESIRE_COAP_DEVICE_UID,0, sizeof(DESIRE_COAP_DEVICE_UID));
    gnrc_netif_t *netif = gnrc_netif_iter(NULL);

    /*
    printf("Board has %d nwk ifaces, id \n",gnrc_netif_numof());
    char addr_str[GNRC_NETIF_L2ADDR_MAXLEN * 3];
    int i = 0;
    while(netif!=NULL) {
        printf("l2_addr [iface =%d]: %s\n", i, gnrc_netif_addr_to_str(netif->l2addr,
                                                   netif->l2addr_len,
                                                   addr_str));
        netif = gnrc_netif_iter(netif);
        i++;
    }
    */
    fmt_bytes_hex(_DESIRE_COAP_DEVICE_UID, netif->l2addr, DESIRE_COAP_DEVICE_UID_BYTES);

}