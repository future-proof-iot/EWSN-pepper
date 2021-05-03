/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_desire_ble_adv EBID advertisment manager
 * @ingroup     sys
 * @brief       Desire BLE EBID advertisement in Carousel mode.
 *
 *
 * @{
 *
 * @file
 *
 * @author      Roudy Dagher <roudy.dagher@inria.fr>
 */

#ifndef DESIRE_BLE_ADV_H
#define DESIRE_BLE_ADV_H

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "timex.h"
#include "ebid.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DESIRE_DEFAULT_SLICE_ROTATION_PERIOD_SEC 20
#define DESIRE_DEFAULT_EBID_ROTATION_PERIOD_SEC (15 * SEC_PER_MIN)

#ifndef DESIRE_STATIC_EBID
    #define DESIRE_STATIC_EBID 0
#endif

/**
 * @brief   Desire footer constant value.
 */
#define MD_VERSION 0xC8

/**
 * @brief   Desire BLE adverisement payload sizethat fits into the Service Data field of the BLE advertisment packet.
 *
 */
#define DESIRE_ADV_PAYLOAD_SIZE               22

/**
 * @brief   The Desire Service data in the BLE Advertisement packet.
 */
typedef union __attribute__((packed)) {
    struct {
        uint16_t service_uuid_16;
        // Header (big endian)
        uint32_t sid_cid;
        // Payload
        uint8_t ebid_slice[EBID_SLICE_SIZE_LONG];
        // Footer little endian
        uint32_t md_version;
    } data;
    uint8_t bytes[DESIRE_ADV_PAYLOAD_SIZE];
} desire_ble_adv_payload_t;

#define MASK_CID 0x3FFFFFFF
#define MASK_SID 0xC0000000
#define SHIFT_SID 30
#define MASK_SID_BYTE 0b11
// Desire packet header manipulation
static inline uint32_t __bswap_32( uint32_t val )
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

static inline void decode_sid_cid(uint32_t header, uint8_t *sid, uint32_t *cid)
{
    uint32_t sid_cid = __bswap_32(header);

    *sid = (sid_cid & MASK_SID) >> SHIFT_SID;
    *cid = sid_cid & MASK_CID;
}

static inline uint32_t encode_sid_cid(uint8_t sid, uint32_t cid)
{
    uint32_t sid_cid = (cid & MASK_CID) + ((sid & MASK_SID_BYTE) << SHIFT_SID);

    return __bswap_32(sid_cid);
}

/**
 * @brief       Constructor for a payload of type @ref desire_ble_adv_payload_t
 *
 * @param[out]     adv_payload    The advertisment payload
 * @param[in]      sid            The slice Id (two signficant bits)
 * @param[in]      cid            The current id associated to the ebid (30 signficant bits)
 *
 */
static inline void desire_ble_adv_payload_build(
    desire_ble_adv_payload_t *adv_payload, uint8_t sid, uint32_t cid,
    const uint8_t *ebid_slice)
{
    adv_payload->data.service_uuid_16 = 0x6666;
    adv_payload->data.sid_cid = encode_sid_cid(sid, cid);                   // header
    memcpy(adv_payload->data.ebid_slice, ebid_slice, EBID_SLICE_SIZE_LONG); // payload
    adv_payload->data.md_version = MD_VERSION;                              // footer
}


/**
 * @brief   Helper for getting the current tx power from the BLE phy
 *
 * @return current tx power in dBm
 *
 */
static inline int8_t ble_current_tx_pwr(void)
{
    extern int ble_phy_txpwr_get(void);

    return (int8_t)ble_phy_txpwr_get(); // tx power is only one byte wide
}

/**
 * @brief       Initialize the advertising module internal structure and advertising thread.
 *
 */
void desire_ble_adv_init(void);

/**
 * @brief       Advertises an EBID following Desire Carousel scheme.
 *
 * The EBID is split in 3 slices with an extra XOR slice. Each slice is advertised every second for a duration slice_adv_time_sec.
 * The EBID is regenerated every ebid_adv_time_sec.
 *
 * @param[in]       slice_adv_time_sec      The rotation period (default to use in @ref DESIRE_DEFAULT_SLICE_ROTATION_PERIOD_SEC)
 * @param[in]       ebid_adv_time_sec       Interval in seconds for renewing the EBID (default to use in @ref DESIRE_DEFAULT_EBID_ROTATION_PERIOD_SEC)
 *
 */
void desire_ble_adv_start(uint16_t slice_adv_time_sec,
                          uint16_t ebid_adv_time_sec);

/**
 * @brief       Stops the advertisement loop.
 *
 */
void desire_ble_adv_stop(void);


#ifdef __cplusplus
}
#endif

#endif /* DESIRE_BLE_ADV_H */
/** @} */
