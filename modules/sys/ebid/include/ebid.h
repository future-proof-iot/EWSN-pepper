/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_ebid Ephemeral Bluetooth Identifiers Generator
 * @ingroup     sys
 * @brief       C25519 based Ephemeral Bluetooth Identifiers Generator (EBID)
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef EBID_H
#define EBID_H

#include <stdint.h>
#include <stdbool.h>

#include "crypto_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   EBID size
 *
 * An EBID is generated from a 32byte c25519 public key
 */
#define EBID_SIZE               C25519_KEY_SIZE
/**
 * @brief   EBID long slice size
 *
 * A 32 byte EBID is divided in two 12byte slices and one 8byte slice
 */
#define EBID_SLICE_SIZE_LONG         12
/**
 * @brief   EBID short slice size
 *
* A 32 byte EBID is divided in two 12byte slices and one 8byte slice
 */
#define EBID_SLICE_SIZE_SHORT   8

/**
 * @name EBID status flags
 *
 * The flags identify the current state of the ebid_t, i.e. which parts
 * are already set.
 *
 * @{
 */
#define EBID_HAS_SLICE_1          (1 << 0)  /**< first slice is set */
#define EBID_HAS_SLICE_2          (1 << 1)  /**< second slice is set */
#define EBID_HAS_SLICE_3          (1 << 2)  /**< third slice is set */
#define EBID_HAS_XOR              (1 << 3)  /**< xor is set */
#define EBID_HAS_ALL              (EBID_HAS_SLICE_1 | EBID_HAS_SLICE_2 | \
                                   EBID_HAS_SLICE_3 | EBID_HAS_XOR)
#define EBID_VALID                (EBID_HAS_ALL)      /**< EBID is valid, are slices and XOR are set */
/** @} */

/**
 * @brief   Convenience EBID structure mapping EBID to individual slices
 */
typedef union __attribute__((packed)) {
    struct __attribute__((packed)) {
        uint8_t ebid_1[EBID_SLICE_SIZE_LONG];       /**< first 12 byte slice */
        uint8_t ebid_2[EBID_SLICE_SIZE_LONG];       /**< second 12 byte slice */
        uint8_t ebid_3[EBID_SLICE_SIZE_LONG];       /**< third 8 byte slice, last 4 bytes are don't care */
    } slice;
    uint8_t u8[EBID_SIZE];                          /**< the complete EBID in an uint8_t buffer */
} ebid_values_t;

/**
 * @brief   EBID status flags
 */
typedef union __attribute__((packed)) {
    uint8_t status;                 /**< ebid status*/
    struct {
        uint8_t ebid_1_set:1;       /**< first slice is set */
        uint8_t ebid_2_set:1;       /**< second slice is set */
        uint8_t ebid_3_set:1;       /**< third slice is set */
        uint8_t ebid_xor_set:1;     /**< xor is set */
    } bit;
} ebid_status_t;

/**
 * @brief   Structure holding EBID and the XOR of the EBID slices
 */
typedef struct {
    ebid_values_t ebid;                     /**< the ebid */
    uint8_t ebid_xor[EBID_SLICE_SIZE_LONG]; /**< the xor of the ebid slices */
} ebid_parts_t;

/**
 * @brief   EBID descriptor
 */
typedef struct __attribute__((packed)) {
    crypto_manager_keys_t* keys;    /**< the key pair matching the ebid, can be NULL*/
    ebid_parts_t parts;             /**< the ebid parts, slices and xor */
    ebid_status_t status;           /**< the ebid status */
} ebid_t;

/**
 * @brief       Initialize an EBID structure, must be called before operating
 *              on any ebit_t structure.
 *
 * @param[inout]    ebid    The preallocated EBID structure to initialize
 *
 * @return      0 on success, -1 otherwise
 */
void ebid_init(ebid_t* ebid);

/**
 * @brief       Generates an EBID from a public/secret key pair
 *
 * @param[inout]    ebid    The preallocated EBID structure to initialize
 * @param[in]       keys    The already generate key pair
 *
 * @return      0 on success, -1 otherwise
 */
int ebid_generate(ebid_t* ebid, crypto_manager_keys_t *keys);

/**
 * @brief       Reconstruct an EBID when a slice or the xor of the slices is
 *              missing
 *
 * @param[inout]    ebid    The ebid to reconstruct
 *
 * @return      0 on success or if all EBID are set, -1 otherwise
 */
int ebid_reconstruct(ebid_t* ebid);

/**
 * @brief       Compare if two EBID match
 *
 * @param[in]       ebid_1      The first ebid to compare
 * @param[in]       ebid_1      The second ebid to compare
 *
 * @return      0 on success or if all EBID are set, -1 otherwise
 */
static inline bool ebid_compare(ebid_t* ebid_1, ebid_t* ebid_2)
{
    return memcmp(ebid_1->parts.ebid.u8, ebid_2->parts.ebid.u8,
                  sizeof(ebid_values_t)) == 0;
}

/**
 * @brief       Returns the EBID buffer from a ebit_t struct
 *
 * @param[in]       ebid        The ebid
 *
 * @return      pointer to the 32 byte ebid buffer
 */
static inline uint8_t* ebid_get(ebid_t* ebid)
{
    return ebid->parts.ebid.u8;
}

/**
 * @brief       Returns the first EBID slice
 *
 * @param[in]       ebid        The ebid
 *
 * @return      pointer to the first 12 byte slice
 */
static inline uint8_t* ebid_get_slice1(ebid_t* ebid)
{
    return ebid->parts.ebid.slice.ebid_1;
}

/**
 * @brief       Returns the second EBID slice
 *
 * @param[in]       ebid        The ebid
 *
 * @return      pointer to the second 12 byte slice
 */
static inline uint8_t* ebid_get_slice2(ebid_t* ebid)
{
    return ebid->parts.ebid.slice.ebid_2;
}

/**
 * @brief       Returns the third EBID slice
 *
 * @param[in]       ebid        The ebid
 *
 * @return      pointer to the third 12 byte slice (last 4 bytes are don't care)
 */
static inline uint8_t* ebid_get_slice3(ebid_t* ebid)
{
    return ebid->parts.ebid.slice.ebid_3;
}

/**
 * @brief       Returns the xor of all EBID slices
 *
 * @param[in]       ebid        The ebid
 *
 * @return      pointer to the 12 bytes xor
 */
static inline uint8_t* ebid_get_xor(ebid_t* ebid)
{
    return ebid->parts.ebid_xor;
}

/**
 * @brief       Sets the first EBID slice
 *
 * @param[inout]     ebid        The ebid to set the first slice
 * @param[inout]     ebid_1      The 12 byte slice
 *
 */
static inline void ebid_set_slice1(ebid_t* ebid, const uint8_t* ebid_1)
{
    memcpy(ebid->parts.ebid.slice.ebid_1, ebid_1, EBID_SLICE_SIZE_LONG);
    ebid->status.status |= EBID_HAS_SLICE_1;
}

/**
 * @brief       Sets the second EBID slice
 *
 * @param[inout]     ebid        The ebid to set the second slice
 * @param[inout]     ebid_2      The 12 byte slice
 *
 */
static inline void ebid_set_slice2(ebid_t* ebid, const uint8_t* ebid_2)
{
    memcpy(ebid->parts.ebid.slice.ebid_2, ebid_2, EBID_SLICE_SIZE_LONG);
    ebid->status.status |= EBID_HAS_SLICE_2;
}

/**
 * @brief       Sets the third EBID slice
 *
 * @param[inout]     ebid        The ebid to set the third slice
 * @param[inout]     ebid_3      An 8 byte buffer holding the third slice
 *
 */
static inline void ebid_set_slice3(ebid_t* ebid, const uint8_t* ebid_3)
{
    memcpy(ebid->parts.ebid.slice.ebid_3, ebid_3, EBID_SLICE_SIZE_SHORT);
    ebid->status.status |= EBID_HAS_SLICE_3;
}

/**
 * @brief       Sets the EBID xor
 *
 * @param[inout]     ebid        The ebid to set the xor
 * @param[inout]     ebid_xor    An 12 byte buffer holding the EBID xor
 *
 */
static inline void ebid_set_xor(ebid_t* ebid, const uint8_t* ebid_xor)
{
    memcpy(ebid->parts.ebid_xor, ebid_xor, EBID_SLICE_SIZE_LONG);
    ebid->status.status |= EBID_HAS_XOR;
}

#ifdef __cplusplus
}
#endif

#endif /* EBID_H */
/** @} */
