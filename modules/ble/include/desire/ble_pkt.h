/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    ble_pkt     Common BLE definitions
 * @ingroup     ble
 * @brief       Desire BLE advertisement packet defintion
 *
 *
 * @{
 *
 * @file
 *
 * @author      Roudy Dagher <roudy.dagher@inria.fr>
 */

#ifndef DESIRE_BLE_PKT_H
#define DESIRE_BLE_PKT_H

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "ebid.h"
#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Desire footer constant value.
 */
#define MD_VERSION 0xC8

/**
 * @brief   Desire BLE service 16 bits uuid.
 */
#define DESIRE_SERVICE_UUID16 0x6666

/**
 * @brief   Desire BLE adverisement payload size that fits into the Service Data
 * field of the BLE advertisementt packet.
 *
 */
#define DESIRE_ADV_PAYLOAD_SIZE               22

/**
 * @brief   The Desire Service data in the BLE Advertisement packet.
 */
typedef union __attribute__((packed)) {
    struct __attribute__((packed)) {
        uint16_t service_uuid_16;                   /**< service identifier */
        // Header (big endian)
        uint32_t sid_cid;
        // Payload
        uint8_t ebid_slice[EBID_SLICE_SIZE_LONG];
        uint8_t md_version;                         /**< version */
        uint8_t tx_power;                           /**< transmit power */
        union {
            uint16_t seed;                          /**< use reserved bytes as seed */
            uint16_t u16;                           /**< reserved bytes */
        } reserved;
    } data;                                         /**< data struct */
    uint8_t bytes[DESIRE_ADV_PAYLOAD_SIZE];         /**< bytes */
} desire_ble_adv_payload_t;

/** @brief CID mask */
#define MASK_CID 0x3FFFFFFF
/** @brief SID mask */
#define MASK_SID 0xC0000000
/** @brief SID shift */
#define SHIFT_SID 30
/** @brief SID byte mask */
#define MASK_SID_BYTE 0b11

/**
 * @brief   Decodes SID, CID from ble pkt header
 *
 * @param header    the header
 * @param sid       SID pointer
 * @param cid       CID pointer
 */
static inline void decode_sid_cid(uint32_t header, uint8_t *sid, uint32_t *cid)
{
    uint32_t sid_cid = byteorder_swapl(header);

    *sid = (sid_cid & MASK_SID) >> SHIFT_SID;
    *cid = sid_cid & MASK_CID;
}

/**
 * @brief   Encodes SID, CID
 *
 * @param sid   SID to encode
 * @param cid   CID to encode
 *
 * @return encoded SID, CID
 */
static inline uint32_t encode_sid_cid(uint8_t sid, uint32_t cid)
{
    uint32_t sid_cid = (cid & MASK_CID) + ((sid & MASK_SID_BYTE) << SHIFT_SID);

    return byteorder_swapl(sid_cid);
}

/**
 * @brief       Constructor for a payload of type @ref desire_ble_adv_payload_t
 *
 * @param[out]     adv_payload    The advertisment payload
 * @param[in]      sid            The slice Id (two signficant bits)
 * @param[in]      cid            The current id associated to the ebid (30 signficant bits)
 * @param[in]      ebid_slice     The EBID slice idx
 * @param[in]      seed           Seed value in advertisement payload
 *
 */
static inline void desire_ble_adv_payload_build(
    desire_ble_adv_payload_t *adv_payload, uint8_t sid, uint32_t cid,
    const uint8_t *ebid_slice, uint16_t seed)
{
    adv_payload->data.service_uuid_16 = DESIRE_SERVICE_UUID16;
    adv_payload->data.sid_cid = encode_sid_cid(sid, cid);                   // header
    adv_payload->data.reserved.seed = seed;
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

#ifdef __cplusplus
}
#endif

#endif /* BLE_PKT_DBG_H */
/** @} */
