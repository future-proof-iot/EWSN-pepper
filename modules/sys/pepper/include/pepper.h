/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_pepper PrEcise Privacy-PresERving Proximity Tracing (PEPPER)
 * @ingroup     sys
 * @brief       PrEcise Privacy-PresERving Proximity Tracing module
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * ## SUBMODULES
 *
 * - `pepper_shell`: includes shell commands to start/stop and configure pepper
 * - `pepper_gatt`: adds a gatt service interface to start/stop and configure pepper
 * - `pepper_stdio_nimble`: add stdio over BLE to provide a shell interface to start
 *    stop and configure PEPPER.
 * - `pepper_current_time`: adds service to listen to BLE current time advertisements
 *    and synchronize to them
 * - `pepper_util`: collection of utilities, currently UID generation and basename
 *    serialization tags.
 *
 * ### Used ZTIMER clocks
 *
 * All used timers can be backed by `ztimer_periph_rtt` allowing the device to
 * sleep. ZTIMER_USEC is not used and should be avoided like the plague.
 *
 * - ZTIMER_MSEC_BASE: the base timer for ZTIMER_MSEC, this timer has the advantage
 *                     of having greater precision than ZTIMER_MSEC, around
 *                     30us when clocked from a 32Khz clock. Altough it forces
 *                     to work with ticks it give us more accurate scheduling of
 *                     events, this is crucial when synchronizing TWR based on
 *                     BLE advertisements and scans since the UWB receiving windows
 *                     are only 2ms.
 * - ZTIMER_MSEC: used when +- millisecond precision, currently unused if not
 *                for logging timestamps
 * - ZTIMER_SEC: used for all relative time, this should be the preffered clock
 *               to use
 * - ZTIMER_EPOCH: it's dervied from ZTIMER_SEC but is dynamically adjusted if
 *                 `current_time` module is used. Timers set on this timer will
 *                  be adjusted accordingly. Currently it is only used for timestamping
 *                  and for scheduling the end of an epoch so that different
 *                  devices schedule these event to roughly the same time.
 *
 */

#ifndef PEPPER_H
#define PEPPER_H

#include "event/thread.h"
#include "ztimer/config.h"
#include "timex.h"
#include "pepper/config.h"
#include "epoch.h"
#include "ed.h"
#if IS_USED(MODULE_TWR)
#include "twr.h"
#endif
#include "ebid.h"
#if IS_USED(MODULE_DESIRE_ADVERTISER)
#include "desire_ble_adv.h"
#include "desire_ble_scan.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   The following event priorities are defined in a way that:
 *              - epoch state machine event come before any other events
 *              - uwb & ble events have same priority
 *              - serialization events are handled last
 */
#ifndef CONFIG_PEPPER_EVENT_PRIO
#define CONFIG_PEPPER_EVENT_PRIO        (EVENT_PRIO_HIGHEST)
#endif
#ifndef CONFIG_PEPPER_LOW_EVENT_PRIO
#define CONFIG_PEPPER_LOW_EVENT_PRIO    (EVENT_PRIO_LOWEST)
#endif
#ifndef CONFIG_UWB_BLE_EVENT_PRIO
#define CONFIG_UWB_BLE_EVENT_PRIO       (EVENT_PRIO_MEDIUM)
#endif

/**
 * @brief   The minimum value for the the TWR offset in milliseconds
 */
#ifndef CONFIG_TWR_MIN_OFFSET_MS
#define CONFIG_TWR_MIN_OFFSET_MS        (100LU)
#endif

/**
 * @brief   The minimum value for the the TWR offset in ticks
 */
#ifndef CONFIG_TWR_MIN_OFFSET_TICKS
#define CONFIG_TWR_MIN_OFFSET_TICKS     ((CONFIG_TWR_MIN_OFFSET_MS)*(CONFIG_ZTIMER_MSEC_BASE_FREQ / \
                                                                     MS_PER_SEC))
#endif

/**
 * @brief   Correction offset when scheduling TWR listen events
 *
 * @note    This wants to account for OS delays
 */
#ifndef CONFIG_TWR_RX_OFFSET_TICKS
#define CONFIG_TWR_RX_OFFSET_TICKS      (-20)
#endif

/**
 * @brief   Correction offset when scheduling TWR request events
 *
 * @note    This wants to account for OS delays and others
 */
#ifndef CONFIG_TWR_TX_OFFSET_TICKS
#define CONFIG_TWR_TX_OFFSET_TICKS      (0)
#endif

/**
 * @brief   Missing in Action (MIA) for encounter, after CONFIG_MIA_TIME_S where
 *          a encounter is not seen over BLE TWR requests will stop being scheduled
 *          to that neighbor on an advertisement completed event.
 */
#ifndef CONFIG_MIA_TIME_S
#define CONFIG_MIA_TIME_S               (30LU)
#endif

/**
 * @brief   Size of the buffer to store base-names or tags for serialized data
 *
 * @note    The tag can be dynamically modified
 */
#ifndef CONFIG_PEPPER_BASE_NAME_BUFFER
#define CONFIG_PEPPER_BASE_NAME_BUFFER  (32)
#endif

/**
 * @brief   If active shell `pepper_start` commands will block until
 *          completion
 */
#ifndef CONFIG_PEPPER_SHELL_BLOCKING
#define CONFIG_PEPPER_SHELL_BLOCKING    0
#endif

/**
 * @brief   Default pepper logfile name
 */
#ifndef CONFIG_PEPPER_LOGS_DIR
#define CONFIG_PEPPER_LOGS_DIR           "/sda/log/"
#endif

/**
 * @brief   Default pepper logfile extension
 */
#ifndef CONFIG_PEPPER_LOG_EXT
#define CONFIG_PEPPER_LOG_EXT           ".txt"
#endif

/**
 * @brief   Set to 1 to force BLE data to be logged for every scan
 */
#ifndef CONFIG_PEPPER_LOG_BLE
#define CONFIG_PEPPER_LOG_BLE           0
#endif

/**
 * @brief   Set to 1 to force TWR data to be logged after every completed exchange
 */
#ifndef CONFIG_PEPPER_LOG_UWB
#define CONFIG_PEPPER_LOG_UWB           0
#endif

/**
 * @brief   Token ID length in bytes
 */
#define PEPPER_UID_LEN          (2)

/**
 * @brief   TWR correction offsets
 */
typedef struct twr_params {
    int16_t rx_offset_ticks;    /* rx/listen event correction */
    int16_t tx_offset_ticks;    /* tx/request event correction */
} twr_params_t;

/**
 * @brief   Epoch parameters
 */
typedef struct __attribute__((__packed__)) {
    uint32_t duration_s;            /**< epoch duration in seconds */
    uint32_t iterations;            /**< epoch iterations count */
} epoch_params_t;

/**
 * @brief   PEPPER start parameters
 */
typedef struct __attribute__((__packed__)) {
    uint32_t epoch_duration_s;      /**< the epoch duration in s */
    uint32_t epoch_iterations;      /**< the epoch iterations, 0 to run forever */
    uint32_t advs_per_slice;        /**< number of advertisements per slice */
    uint32_t adv_itvl_ms;           /**< advertisement interval in milliseconds */
    uint32_t scan_itvl_ms;          /**< scan interval in milliseconds */
    uint32_t scan_win_ms;           /**< scan window in milliseconds */
    bool align;                     /**< align end of epoch event with epoch_duration_s */
} pepper_start_params_t;

/**
 * @brief   PEPPER status enum
 */
typedef enum {
    PEPPER_STOPPED,     /**< PEPPER is stopped */
    PEPPER_PAUSED,      /**< PEPPER is paused */
    PEPPER_RUNNING,     /**< PEPPER is active */
} controller_status_t;

/**
 * @brief   PEPPER controller data
 */
typedef struct controller {
    ebid_t ebid;                        /**> the local EBID */
    ed_list_t ed_list;                  /**> encounter data list */
    ed_memory_manager_t ed_mem;         /**> encounter data memory manager */
#if IS_USED(MODULE_TWR)
    twr_event_mem_manager_t twr_mem;    /**> twr events memory manager */
    twr_params_t twr_params;            /**> twr parameters */
#endif
    crypto_manager_keys_t keys;         /**> current pub, priv key pair */
    uint32_t start_time;                /**> time stamp of the current epoch
                                             start time taken from ZTIMER_SEC */
    epoch_data_t data;                  /**> epoch_data structure to populate at
                                             the end of an epoch */
    mutex_t lock;                       /**> lock to prevent multiple calls to
                                             pepper_start */
    epoch_params_t epoch;               /**> current epoch parameters */
#if IS_USED(MODULE_DESIRE_ADVERTISER)
    adv_params_t adv;                   /**> configured advertisement parameters */
#endif
#if IS_USED(MODULE_DESIRE_SCANNER)
    ble_scan_params_t scan;             /**> scan configuration */
#endif
    controller_status_t status;         /**> controller status */
} controller_t;

/**
 * @brief   Initialize the PEPPER service
 */
void pepper_init(void);

/**
 * @brief   Starts PEPPER with provided parameters
 *
 * @param   params  PEPPER parameters
 */
void pepper_start(pepper_start_params_t *params);

/**
 * @brief   Stops PEPPER, this clears all encounter data, twr data should clear
 *          itself
 */
void pepper_stop(void);

/**
 * @brief   Pauses pepper, stops ongoing activity but if it is active state
 *          is set to passed and can be resumed on @ref pepper_resume
 */
void pepper_pause(void);

/**
 * @brief   If pepper was not STOPPED then it resumes operations with the
 *          previous configuration
 *
 * @param[in]   align   Align end of epoch event according to epoch.duration_s
 */
void pepper_resume(bool align);

/**
 * @brief   Query pepper status
 *
 * @return true     if pepper is active or paused
 * @return false    if pepper is stopped
 */
bool pepper_is_active(void);

/**
 * @brief   Returns pointer to internal controller descriptor
 * @internal
 *
 * @return  pointer to controller data descriptor
 */
controller_t *pepper_get_controller(void);

/**
 * @brief   Configure an offset to be added when scheduling TWR requests events
 *
 * @param   ticks     ticks offset
 */
void pepper_twr_set_rx_offset(int16_t ticks);

/**
 * @brief   Configure an offset to be added when scheduling TWR listen events
 *
 * @param   ticks     ticks offset
 */
void pepper_twr_set_tx_offset(int16_t ticks);

/**
 * @brief   Returns the configured offset
 *
 * @return  the configured offset
 */
int16_t pepper_twr_get_rx_offset(void);

/**
 * @brief   Returns the configured offset
 *
 * @return  the configured offset
 */
int16_t pepper_twr_get_tx_offset(void);

#if IS_USED(MODULE_PEPPER_UTIL)
/**
 * @brief   Configure the basename string to be added when serializing and logging
 *
 * @note    This can be used for data annotation
 *
 * @param[in]   base_name   input base name string
 *
 * @return  0 on success, -1 if string length > CONFIG_PEPPER_BASE_NAME_BUFFER
 */
int pepper_set_serializer_bn(char *base_name);
/**
 * @brief   Returns the current basename
 *
 * @return  the basename
 */
char *pepper_get_serializer_bn(void);
#else
static inline int pepper_set_serializer_bn(char *base_name)
{
    (void)base_name;
    return -1;
}
static inline char *pepper_get_serializer_bn(void)
{
    return NULL;
}
#endif

/**
 * @brief   Returns the devices UID
 *
 * @return  Pointer to the uid string
 */
char *pepper_get_uid(void);

/**
 * @brief   PEPPER uid service initialization
 */
void pepper_uid_init(void);

/**
 * @brief   PEPPER gatt interface initialization
 */
void pepper_gatt_init(void);

/**
 * @brief   PEPPER stdio nimble initialization
 *
 * @note    This overrides the default initialization so that the advertisement
 *          name is set to PEPPER UID
 */
void pepper_stdio_nimble_init(void);

/**
 * @brief   PEPPER time synchronization service initialization
 *
 * This sets up the callback to execute before and after adjusting time.
 */
void pepper_current_time_init(void);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_H */
/** @} */
