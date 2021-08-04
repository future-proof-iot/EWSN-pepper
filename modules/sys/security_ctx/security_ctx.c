/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_security_ctx
 * @{
 *
 * @file
 * @brief       Crypto Context implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include "assert.h"
#include "security_ctx.h"
#include "cose.h"
#include "tinycrypt/constants.h"
#include "tinycrypt/hkdf.h"

void security_ctx_init(security_ctx_t *ctx, uint8_t *send_id,
                       size_t send_id_len, uint8_t *recv_id,
                       size_t recv_id_len)
{
    assert(recv_id_len <= SECURITY_CTX_ID_MAX_LEN);
    assert(send_id_len <= SECURITY_CTX_ID_MAX_LEN);
    memset(ctx->send_ctx_key, '\0', SECURITY_CTX_KEY_LEN);
    memset(ctx->recv_ctx_key, '\0', SECURITY_CTX_KEY_LEN);
    memset(ctx->common_iv, '\0', SECURITY_CTX_COMMON_IV_LEN);
    ctx->send_id = send_id;
    ctx->recv_id = recv_id;
    ctx->send_id_len = send_id_len;
    ctx->recv_id_len = recv_id_len;
    ctx->seqnr = 0;
    ctx->valid = false;
}

int security_ctx_key_gen(security_ctx_t *ctx,
                         uint8_t *salt, size_t salt_len,
                         uint8_t *secret, size_t secret_len)
{
    uint8_t prk[TC_SHA256_DIGEST_SIZE];

    int ret = tc_hkdf_extract(secret, secret_len, salt, salt_len, prk);

    if (ret != TC_CRYPTO_SUCCESS) {
        return -1;
    }
    ret = tc_hkdf_expand(prk, NULL, 0, SECURITY_CTX_COMMON_IV_LEN,
                         ctx->common_iv);
    if (ret != TC_CRYPTO_SUCCESS) {
        return -1;
    }
    ret = tc_hkdf_expand(prk, ctx->send_id, ctx->send_id_len,
                         SECURITY_CTX_KEY_LEN, ctx->send_ctx_key);
    if (ret != TC_CRYPTO_SUCCESS) {
        return -1;
    }
    ret = tc_hkdf_expand(prk, ctx->recv_id, ctx->recv_id_len,
                         SECURITY_CTX_KEY_LEN, ctx->recv_ctx_key);
    if (ret != TC_CRYPTO_SUCCESS) {
        return -1;
    }

    /* valid security context */
    ctx->valid = true;

    return 0;
}

/* TODO: libcose does not support not making the nonce public */
void security_ctx_gen_nonce(security_ctx_t *ctx, uint8_t *ctx_id,
                            size_t ctx_id_len, uint8_t *nonce)
{
    uint8_t partial_iv[SECURITY_CTX_NONCE_LEN];
    memset(partial_iv, '\0', sizeof(partial_iv));
    memcpy(partial_iv + (SECURITY_CTX_NONCE_LEN - 5 - ctx_id_len), ctx_id, ctx_id_len);
    partial_iv[SECURITY_CTX_NONCE_LEN -2] = ctx->seqnr >> 8;
    partial_iv[SECURITY_CTX_NONCE_LEN -1] = ctx->seqnr & 0xFF;
    ctx->seqnr++;
    for (uint8_t i = 0; i < SECURITY_CTX_NONCE_LEN; i++) {
        *nonce++ = partial_iv[i] ^ ctx->common_iv[i];
    }
}

int security_ctx_encode(security_ctx_t *ctx, uint8_t *data, size_t data_len,
                        uint8_t *buf, size_t buf_len, uint8_t **out)
{
    /* setup cose key */
    cose_key_t key;

    cose_key_init(&key);
    cose_key_set_kid(&key, ctx->send_id, ctx->send_id_len);
    cose_key_set_keys(&key, 0, COSE_ALGO_AESCCM_16_64_128, NULL, NULL,
                      ctx->send_ctx_key);

    /* generate nonce */
    uint8_t nonce[SECURITY_CTX_NONCE_LEN];
    security_ctx_gen_nonce(ctx, ctx->send_id, ctx->send_id_len,
                           nonce);

    /* setup cose encrypt */
    cose_encrypt_t crypt;
    crypt.ext_aad = NULL;
    crypt.ext_aad_len = 0;
    cose_encrypt_init(&crypt, COSE_FLAGS_ENCRYPT0);
    cose_encrypt_add_recipient(&crypt, &key);
    cose_encrypt_set_payload(&crypt, data, data_len);
    cose_encrypt_set_algo(&crypt, COSE_ALGO_DIRECT);

    /* encrypt and encode */
    return cose_encrypt_encode(&crypt, buf, buf_len, nonce, out);
}

int security_ctx_decode(security_ctx_t *ctx, uint8_t *in, size_t in_len,
                        uint8_t *buf, size_t buf_len, uint8_t *out, size_t* olen)
{
    cose_key_t key;

    /* setup cose key */
    cose_key_init(&key);
    cose_key_set_kid(&key, ctx->recv_id, ctx->recv_id_len);
    cose_key_set_keys(&key, 0, COSE_ALGO_AESCCM_16_64_128, NULL, NULL,
                      ctx->recv_ctx_key);
    cose_encrypt_dec_t decrypt;
    cose_encrypt_decode(&decrypt, in, in_len);
    return cose_encrypt_decrypt(&decrypt, NULL, &key, buf, buf_len, out, olen);
}
