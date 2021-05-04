#ifndef BLE_PKT_DBG_H
#define BLE_PKT_DBG_H

#include "host/ble_hs.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void dbg_dump_buffer(const char *prefix, const uint8_t *buf,
                               size_t size,
                               char suffix)
{
    printf("%s", prefix);
    for (unsigned int i = 0; i < size; i++) {
        printf("%.2X", buf[i]);
        putchar((i < (size - 1))?':':suffix);
    }
}

static inline void dbg_print_addr(const ble_addr_t *addr)
{
    printf("%02x", (int)addr->val[5]);
    for (int i = 4; i >= 0; i--) {
        printf(":%02x", addr->val[i]);
    }

    switch (addr->type) {
    case BLE_ADDR_PUBLIC:       printf(" (PUBLIC)");   break;
    case BLE_ADDR_RANDOM:       printf(" (RANDOM)");   break;
    case BLE_ADDR_PUBLIC_ID:    printf(" (PUB_ID)");   break;
    case BLE_ADDR_RANDOM_ID:    printf(" (RAND_ID)");  break;
    default:                    printf(" (UNKNOWN)");  break;
    }
}

static inline void dbg_print_type(uint8_t type)
{
    switch (type) {
    case BLE_HCI_ADV_TYPE_ADV_IND:
        printf(" [IND]");
        break;
    case BLE_HCI_ADV_TYPE_ADV_DIRECT_IND_HD:
        printf(" [DIRECT_IND_HD]");
        break;
    case BLE_HCI_ADV_TYPE_ADV_SCAN_IND:
        printf(" [SCAN_IND]");
        break;
    case BLE_HCI_ADV_TYPE_ADV_NONCONN_IND:
        printf(" [NONCONN_IND]");
        break;
    case BLE_HCI_ADV_TYPE_ADV_DIRECT_IND_LD:
        printf(" [DIRECT_IND_LD]");
        break;
    default:
        printf(" [INVALID]");
        break;
    }
}


#ifdef __cplusplus
}
#endif

#endif /* NET_BLUETIL_AD_H */
/** @} */