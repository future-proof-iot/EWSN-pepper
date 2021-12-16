/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_json_encoder
 * @{
 *
 * @file
 * @brief       JSONEncoder implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "json_encoder.h"
#include "fmt.h"

#if IS_USED(MODULE_JSON_ENCODER_FLOAT_FULL)
static int _norm_f(double *val)
{
    double value = *val;
    const double pos_exp_thresh = 1e7;
    const double neg_exp_thresh = 1e-5;
    int exp = 0;

    if (value >= pos_exp_thresh) {
        if (value >= 1e256) {
            value /= 1e256;
            exp += 256;
        }
        if (value >= 1e128) {
            value /= 1e128;
            exp += 128;
        }
        if (value >= 1e64) {
            value /= 1e64;
            exp += 64;
        }
        if (value >= 1e32) {
            value /= 1e32;
            exp += 32;
        }
        if (value >= 1e16) {
            value /= 1e16;
            exp += 16;
        }
        if (value >= 1e8) {
            value /= 1e8;
            exp += 8;
        }
        if (value >= 1e4) {
            value /= 1e4;
            exp += 4;
        }
        if (value >= 1e2) {
            value /= 1e2;
            exp += 2;
        }
        if (value >= 1e1) {
            value /= 1e1;
            exp += 1;
        }
    }

    if (value > 0 && value <= neg_exp_thresh) {
        if (value < 1e-255) {
            value *= 1e256;
            value *= 1e128;
            exp -= 128;
        }
        if (value < 1e-63) {
            value *= 1e64;
            exp -= 64;
        }
        if (value < 1e-31) {
            value *= 1e32;
            exp -= 32;
        }
        if (value < 1e-15) {
            value *= 1e16;
            exp -= 16;
        }
        if (value < 1e-7) {
            value *= 1e8;
            exp -= 8;
        }
        if (value < 1e-3) {
            value *= 1e4;
            exp -= 4;
        }
        if (value < 1e-1) {
            value *= 1e2;
            exp -= 2;
        }
        if (value < 1e0) {
            value *= 1e1;
            exp -= 1;
        }
    }

    return exp;
}


static void _split_float(double value, uint32_t *integer, uint32_t *fraction, int16_t *exponent)
{
    uint32_t int_part, dec_part;
    int16_t exp;

    exp = _norm_f(&value);
    int_part = (uint32_t)value;
    double rem = value - int_part;

    rem *= 1e9;
    dec_part = (uint32_t)rem;

    rem -= dec_part;
    if (rem >= 0.5) {
        dec_part++;
        if (dec_part >= 1000000000) {
            dec_part = 0;
            int_part++;
            if (exp != 0 && int_part >= 10) {
                exp++;
                int_part = 1;
            }
        }
    }

    int width = 9;
    while( dec_part % 10 == 0 && width > 0) {
        dec_part /= 10;
        width--;
    }

    *exponent = exp;
    *integer = int_part;
    *fraction = dec_part;
}

static size_t fmt_float_full(char *out, double value){
    unsigned negative = (value < 0);

    if (isnan(value)) {
        return fmt_str(out, "nan");
    }

    if (isinf(value)) {
        return fmt_str(out, "inf");
    }

    if (negative) {
        value = -value;
        if (out) {
            *out++ = '-';
        }
    }

    uint32_t integer, fraction;
    int16_t exp;
    _split_float(value, &integer, &fraction, &exp);

    size_t res = fmt_u32_dec(out, integer);
    if (fraction) {
        if (out) {
            out += res;
            *out++ = '.';
        }
        res++;
        size_t tmp = fmt_u32_dec(out, fraction);
        res += tmp;
        if (out) {
            out += tmp;
        }
    }

    if (exp < 0) {
        size_t tmp = fmt_str(out, "e-");
        res += tmp;
        if (out) {
            out += tmp;
        }
        fmt_u16_dec(out, -exp);
    }

    if (exp > 0) {
        if (out) {
            *out++ = 'e';
        }
        res++;
        size_t tmp = fmt_u16_dec(out, exp);
        res += tmp;
        if (out) {
            out += tmp;
        }
    }

    return res;
}
#endif

int json_array_open(json_encoder_t *enc)
{
    enc->len += fmt_char(enc->cur++, '[');
    return 1;
}

int json_array_close(json_encoder_t *enc)
{
    /* a ',' is always appended so remove from the last map */
    fmt_char(enc->cur - 1, ']');
    enc->len += fmt_char(enc->cur++, '\0');
    return 1;
}

void json_encoder_init(json_encoder_t *enc, char *buf, size_t len)
{
    enc->len = 0;
    enc->cur = buf;
    enc->end = buf + len;
}

size_t json_encoder_len(json_encoder_t *enc)
{
    return enc->len;
}

size_t json_encoder_end(json_encoder_t *enc)
{
    fmt_char(enc->cur - 1, '\n');
    enc->len += fmt_char(enc->cur, '\0');
    return enc->len;
}

int json_dict_open(json_encoder_t *enc)
{
    enc->len += fmt_char(enc->cur++, '{');
    return 1;
}

int json_dict_close(json_encoder_t *enc)
{
    fmt_char(enc->cur - 1, '}');
    enc->len += fmt_char(enc->cur++, ',');
    return 1;
}

int json_dict_key(json_encoder_t *enc, const char *key)
{
    int len = 0;

    len += fmt_char(enc->cur, '\"');
    len += fmt_str(enc->cur + len, key);
    len += fmt_char(enc->cur + len, '\"');
    len += fmt_char(enc->cur + len, ':');
    enc->len += len;
    enc->cur += len;
    return len;
}

int json_string(json_encoder_t *enc, const char *str)
{
    int len = 0;

    len += fmt_char(enc->cur, '\"');
    len += fmt_str(enc->cur + len, str);
    len += fmt_char(enc->cur + len, '\"');
    len += fmt_char(enc->cur + len, ',');
    enc->len += len;
    enc->cur += len;
    return len;
}

int json_s32(json_encoder_t *enc, int32_t val)
{
    int len = fmt_s32_dec(enc->cur, val);

    len += fmt_char(enc->cur + len, ',');
    enc->len += len;
    enc->cur += len;
    return len;
}

int json_u32(json_encoder_t *enc, uint32_t val)
{
    int len = fmt_u32_dec(enc->cur, val);

    len += fmt_char(enc->cur + len, ',');
    enc->len += len;
    enc->cur += len;
    return len;
}

int json_s64(json_encoder_t *enc, int64_t val)
{
    int len = fmt_s64_dec(enc->cur, val);

    len += fmt_char(enc->cur + len, ',');
    enc->len += len;
    enc->cur += len;
    return len;
}

int json_u64(json_encoder_t *enc, uint64_t val)
{
    int len = fmt_u64_dec(enc->cur, val);

    len += fmt_char(enc->cur + len, ',');
    enc->len += len;
    enc->cur += len;
    return len;
}

int json_float(json_encoder_t *enc, float val)
{
    int len = 0;


#if IS_USED(MODULE_JSON_ENCODER_FLOAT_FULL)
    len += fmt_float_full(enc->cur + len, val);
#else
    len += fmt_float(enc->cur + len, val, 7);
#endif
    len += fmt_char(enc->cur + len, ',');
    enc->len += len;
    enc->cur += len;
    return len;
}

int json_dict_string(json_encoder_t *enc, const char *key, const char *val)
{
    int len = 0;

    len += json_dict_open(enc);
    len += json_dict_key(enc, key);
    len += json_string(enc, val);
    len += json_dict_close(enc);
    return len;
}

int json_hexarray(json_encoder_t *enc, uint8_t *vals, size_t size)
{
    int len = 0;

    len += fmt_char(enc->cur + len, '\"');
    len += fmt_bytes_hex(enc->cur + len, vals, size);
    len += fmt_char(enc->cur + len, '\"');
    len += fmt_char(enc->cur + len, ',');
    enc->cur += len;
    enc->len += len;
    return len;
}
