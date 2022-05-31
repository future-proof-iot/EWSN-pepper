/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_json_encoder JSONEncoder
 * @ingroup     sys
 * @brief       A simple JSON Encoder
 *
 * @{
 *
 * @file
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 */

#ifndef JSON_ENCODER_H
#define JSON_ENCODER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief JSON encoder context
 */
typedef struct json_encoder {
    char *cur;  /**< Current position in the buffer */
    char *end;  /**< end of the buffer */
    size_t len; /**< Length in bytes of supplied JSON data. Incremented
                     separate from the buffer check  */
} json_encoder_t;

/**
 * @brief Init the encoding of a JSON
 * @note this opens the JSON array
 *
 * @param[in] enc       The JSON encoder context
 * @param[in] buf       The buffer to write into
 * @param[in] len       The length of the buffer
 */
void json_encoder_init(json_encoder_t *enc, char *buf, size_t len);

/**
 * @brief   Return current encoded len
 *
 * @param[in] enc       The JSON encoder context
 *
 * @return the size of the encoded data
 */
size_t json_encoder_len(json_encoder_t *enc);

/**
 * @brief   Finalize encoding
 *
 * @param[in] enc       The JSON encoder context
 *
 * @return the size of the encoded data
 */
size_t json_encoder_end(json_encoder_t *enc);

/**
 * @brief  Encodes a formatted open of an array result.
 *
 * @param[in] enc       The JSON encoder context
 */
int json_array_open(json_encoder_t *enc);

/**
 * @brief  Encodes a formatted close of an array result.
 *
 * @param[in] enc       The JSON encoder context
 */
int json_array_close(json_encoder_t *enc);

/**
 * @brief  Encodes a formatted open of a dictionary result.
 *
 * @param[in] enc       The JSON encoder context
 */
int json_dict_open(json_encoder_t *enc);

/**
 * @brief  Encodes formatted close of a dictionary result.
 *
 * @param[in] enc       The JSON encoder context
 */
int json_dict_close(json_encoder_t *enc);

/**
 * @brief  Encodes a formatted open of a dictionary result.
 *
 * A value must follow.
 *
 * @param[in] enc       The JSON encoder context
 * @param[in] key       The key of the dictionary.
 */
int json_dict_key(json_encoder_t *enc, const char *key);

/**
 * @brief  Encodes a formatted string string.
 *
 * @param[in] enc       The JSON encoder context
 * @param[in] str       The string to output.
 */
int json_string(json_encoder_t *enc, const char *str);

/**
 * @brief  Encodes a signed 32 bit integer.
 *
 * @param[in] enc       The JSON encoder context
 * @param[in] val       The value to output.
 */
int json_s32(json_encoder_t *enc, int32_t val);

/**
 * @brief  Encodes an unsigned 32 bit integer.
 *
 * @param[in] enc       The JSON encoder context
 * @param[in] val       The value to output.
 */
int json_u32(json_encoder_t *enc, uint32_t val);

/**
 * @brief  Encodes a signed 64 bit integer.
 *
 * @param[in] enc       The JSON encoder context
 * @param[in] val       The value to output.
 */
int json_s64(json_encoder_t *enc, int64_t val);

/**
 * @brief  Encodes formatted result unsigned 64 bit integer.
 *
 * @param[in] enc       The JSON encoder context
 * @param[in] val       The value to output.
 */
int json_u64(json_encoder_t *enc, uint64_t val);

/**
 * @brief  Encodes a formatted float result of varied precision.
 *
 * @param[in] enc       The JSON encoder context
 * @param[in] val       The value to output.
 */
int json_float(json_encoder_t *enc, float val);

/**
 * @brief   Encodes a dict with string data.
 *
 * @param[in] enc       The JSON encoder context
 * @param[in] key       A dictionary key.
 * @param[in] val       A string value of the dictionary
 */
int json_dict_string(json_encoder_t *enc, const char *key, const char *val);

/**
 * @brief   Encodes a byte array as a hex string
 *
 * @param[in] enc       The JSON encoder context
 * @param[in] vals      Byte array
 * @param[in] size       Array size
 */
int json_hexarray(json_encoder_t *enc, uint8_t *vals, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* JSON_ENCODER_H */
/** @} */
