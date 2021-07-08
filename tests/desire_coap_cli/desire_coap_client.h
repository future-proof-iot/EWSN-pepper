#ifndef DESIRE_COAP_CLIENT_H
#define DESIRE_COAP_CLIENT_H

#include <string.h>
#include <stdint.h>
#include <stdbool.h>



#ifdef __cplusplus
extern "C" {
#endif


#define DESIRE_COAP_DEVICE_UID_BYTES 2

extern const char* DESIRE_COAP_DEVICE_UID;

/**
 * @brief       Initialize the coap client module.
 *
 */
void desire_coap_cli_init(void); 

/**
 * @brief       Sets the coap server endpoint.
 *
 * @param[in]       host                    The coap server IPv6 address
 * @param[in]       port                    The coap server port number (eg. 5683)
 */
void desire_coap_cli_set_endpoint(const char* host, uint16_t port); 

/**
 * @brief       Prints the coap server endpoint.
 *
 */
void desire_coap_cli_print_endpoint(void); 


#ifdef __cplusplus
}
#endif

#endif /* DESIRE_BLE_ADV_H */
/** @} */
