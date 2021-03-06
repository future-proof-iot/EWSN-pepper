/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    ble_pkt_dbg  BLE Packet Debug Utils
 * @ingroup     ble_pkt
 * @brief       BLE debug utilities
 *
 * @{
 *
 * @file
 *
 */

#ifndef BLE_PKT_DBG_H
#define BLE_PKT_DBG_H

#include "host/ble_hs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Dump buffer in host order to stdio with optional prefix and suffix
 *
 * @param prefix    the prefix string
 * @param buf       the buffer to dump
 * @param size      size of buffer to dump
 * @param suffix    the suffix string
 */
static inline void dbg_dump_buffer(const char *prefix, const uint8_t *buf,
                                   size_t size,
                                   char suffix)
{
    printf("%s", prefix);
    for (unsigned int i = 0; i < size; i++) {
        printf("%.2X", buf[i]);
        putchar((i < (size - 1))?':':suffix);
    }
    if (size == 0) {
        putchar(suffix);
    }
}

/**
 * @brief   Dump in network order
 *
 * @param prefix    the prefix string
 * @param buf       the buffer to dump
 * @param size      size of buffer to dump
 * @param suffix    the suffix string
 */
static inline void dbg_reverse_dump_buffer(const char *prefix,
                                           const uint8_t *buf,
                                           size_t size,
                                           char suffix)
{
    printf("%s", prefix);
    for (int i = size - 1; i >= 0; i--) {
        printf("%.2X", buf[i]);
        putchar((i > 0)?':':suffix);
    }
    if (size == 0) {
        putchar(suffix);
    }
}

/**
 * @brief   Returns address type as astring
 *
 * @param   addr_type     the address type
 * @return address type
 */
static inline char *dbg_parse_ble_addr_type(uint8_t addr_type)
{
    switch (addr_type) {
    case BLE_ADDR_PUBLIC:       return "(PUBLIC)";
    case BLE_ADDR_RANDOM:       return "(RANDOM)";
    case BLE_ADDR_PUBLIC_ID:    return "(PUB_ID)";
    case BLE_ADDR_RANDOM_ID:    return "(RAND_ID)";
    default:                    return "(UNKNOWN)";
    }
}

/**
 * @brief   Returns the advertisement type as a string
 *
 * @param   type    the type
 * @return the advertisement type
 */
static inline char *dbg_parse_ble_adv_type(uint8_t type)
{
    switch (type) {
    case BLE_HCI_ADV_TYPE_ADV_IND: return "[IND]";
    case BLE_HCI_ADV_TYPE_ADV_DIRECT_IND_HD: return "[DIRECT_IND_HD]";
    case BLE_HCI_ADV_TYPE_ADV_SCAN_IND: return "[SCAN_IND]";
    case BLE_HCI_ADV_TYPE_ADV_NONCONN_IND: return "[NONCONN_IND]";
    case BLE_HCI_ADV_TYPE_ADV_DIRECT_IND_LD: return "[DIRECT_IND_LD]";
    default: return " [INVALID]";
    }
}

/**
 * @brief   Prints a ble address
 *
 * @param   addr    the address
 */
static inline void dbg_print_ble_addr(const ble_addr_t *addr)
{
    dbg_reverse_dump_buffer("ble_addr = ", addr->val, 6, ' ');
    printf("%s\n", dbg_parse_ble_addr_type(addr->type));
}

#ifdef __cplusplus
}
#endif

#endif /* BLE_PKT_DBG_H */
/** @} */
