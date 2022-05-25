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
 * @param enc JSON encoder.
 * @param buf buffer to write into
 * @param len length of the buffer
 */
void json_encoder_init(json_encoder_t *enc, char *buf, size_t len);

/**
 * @brief End the encoding of a JSON
 *
 * @note this closes the JSON array
 *
 * @param enc JSON encoder.
 *
 * @return Size of the encoded data.
 */
size_t json_encoder_end(json_encoder_t *enc);

/**
 * @brief Return the length of the encoded JSON
 *
 * @param enc JSON encoder.
 *
 * @return Size of the encoded data.
 */
size_t json_encoder_len(json_encoder_t *enc);

int json_array_open(json_encoder_t *enc);
int json_array_close(json_encoder_t *enc);
void json_encoder_init(json_encoder_t *enc, char *buf, size_t len);
size_t json_encoder_len(json_encoder_t *enc);
size_t json_encoder_end(json_encoder_t *enc);
int json_dict_open(json_encoder_t *enc);
int json_dict_close(json_encoder_t *enc);
int json_dict_key(json_encoder_t *enc, const char *key);
int json_string(json_encoder_t *enc, const char *str);
int json_s32(json_encoder_t *enc, int32_t val);
int json_u32(json_encoder_t *enc, uint32_t val);
int json_s64(json_encoder_t *enc, int64_t val);
int json_u64(json_encoder_t *enc, uint64_t val);
int json_float(json_encoder_t *enc, float val);
int json_dict_string(json_encoder_t *enc, const char *key, const char *val);
int json_hexarray(json_encoder_t *enc, uint8_t *vals, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* JSON_ENCODER_H */
/** @} */
