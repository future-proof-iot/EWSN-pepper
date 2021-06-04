/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    ble_include  BLE Wall Clock Time definition
 * @ingroup     ble
 * @brief       Desire BLE Wall Clock Time advertisement packet defintion
 *
 *
 * @{
 *
 * @file
 *
 * @author      Roudy Dagher <roudy.dagher@inria.fr>
 */

#ifndef TIME_BLE_PKT_H
#define TIME_BLE_PKT_H

#include <stdio.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief   Wall Clock Time BLE service 16 bits uuid.
 */
#define CURRENT_TIME_SERVICE_UUID16 0x6665

/**
 * @brief   Wall Clock Time  BLE adverisement payload sizet hat fits into the Service Data field of the BLE advertisment packet.
 *
 */
#define CURRENT_TIME_ADV_PAYLOAD_SIZE  12

/**
 * @brief   The Desire Service data in the BLE Advertisement packet.
 */
typedef union __attribute__((packed)) {
    struct __attribute__((packed)) {
        /* service data header */
        uint16_t service_uuid_16;
        /* service data  payload */
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hours;
        uint8_t minutes;
        uint8_t seconds;
        uint8_t day_of_week; //  From 1 (Monday) to 7 (Sunday)
        uint8_t fraction256; // not used
        uint8_t adjust_reason; // not used
    } data;
    uint8_t bytes[CURRENT_TIME_ADV_PAYLOAD_SIZE];
} current_time_ble_adv_payload_t;


static inline void dbg_print_curr_time_pkt(const current_time_ble_adv_payload_t *pkt)
{
    printf("[Current TIme Packet] Service uuid16  = 0x%X\n", pkt->data.service_uuid_16);
    printf("\t .year = %d\n", pkt->data.year);
    printf("\t .month = %d\n", pkt->data.month);
    printf("\t .day = %d\n", pkt->data.day);
    printf("\t .hours = %d\n", pkt->data.hours);
    printf("\t .minutes = %d\n", pkt->data.minutes);
    printf("\t .seconds = %d\n", pkt->data.seconds);
    printf("\t .day_of_week = %d\n", pkt->data.day_of_week);
    printf("\t .fraction256 = %d\n", pkt->data.fraction256);
    printf("\t .adjust_reason = %d\n", pkt->data.adjust_reason);
}

#ifdef __cplusplus
}
#endif

#endif /* TIME_BLE_PKT_H */
/** @} */
