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

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "periph/rtc.h"

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

/**
 * @brief   Callback signature triggered by this module for each discovered
 *          advertising Current Time Service packet
 *
 * @param[in] adv_payload   Decoded time payload
 */
typedef void (*time_update_cb_t) (const current_time_ble_adv_payload_t *
                                  adv_payload);

/**
 * @brief   Parses ble advertisemnt current time payload and return time struct
 *
 * @param[in]       payload current_time advertisement payload
 * @param[inout]    time    time structure loaded from advertisement data
 *
 */
static inline void current_time_ble_adv_parse(const current_time_ble_adv_payload_t *payload, struct tm *time)
{
    time->tm_year = payload->data.year - 1900;
    time->tm_mon = payload->data.month - 1;
    time->tm_mday = payload->data.day;
    time->tm_hour = payload->data.hours;
    time->tm_min = payload->data.minutes;
    time->tm_sec = payload->data.seconds;
}

/**
 * @brief   Loads current time adv_payload from epoch
 *
 * @param[inout]    payload current_time advertisement payload
 * @param[in]       epoch   the epoch timestamp
 */
static inline void current_time_ble_adv_init(current_time_ble_adv_payload_t *payload, uint32_t epoch)
{
    struct tm time;
    rtc_localtime(epoch, &time);
    memset(payload->bytes, 0, CURRENT_TIME_ADV_PAYLOAD_SIZE);
    payload->data.service_uuid_16 = CURRENT_TIME_SERVICE_UUID16;
    payload->data.year = (uint16_t)(time.tm_year + 1900);
    payload->data.month = (uint8_t)(time.tm_mon + 1);
    payload->data.day =  (uint8_t)(time.tm_mday);
    payload->data.hours = (uint8_t)(time.tm_hour);
    payload->data.minutes = (uint8_t)(time.tm_min);
    payload->data.seconds = (uint8_t)(time.tm_sec);
    payload->data.day_of_week = (uint8_t)(time.tm_wday);
}

/**
 * @brief   Parses ble current time advertisement into epoch timestamp
 *
 * @param[in]       payload current_time advertisement payload
 * @return          the parsed epoch
 */
static inline uint32_t current_time_ble_adv_parse_epoch(const current_time_ble_adv_payload_t *payload)
{
    struct tm time;
    current_time_ble_adv_parse(payload, &time);
    return rtc_mktime(&time);
}

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
