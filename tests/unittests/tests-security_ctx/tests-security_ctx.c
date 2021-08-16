#include <string.h>

#include "embUnit.h"
#include "security_ctx.h"

#define ENABLE_DEBUG    0
#include "debug.h"

static uint8_t alice_id[] = { 0xcc, 0xd1 };
static uint8_t bob_id[] = { 0xac, 0xe2 };

static uint8_t salt[] = {
    0xea, 0xea, 0xd4, 0x48, 0xe0, 0x56, 0xef, 0x83
};
static uint8_t secret[] = {
    0x16, 0x6c, 0x45, 0xab, 0xb8, 0xd6, 0xdb, 0xe5,
    0xd7, 0x71, 0xeb, 0x1d, 0x8b, 0x20, 0x21, 0xa4
};
static uint8_t bob_send_key[] = {
    0xc5, 0x6b, 0x81, 0x63, 0x41, 0xe9, 0xd3, 0x02,
    0xce, 0x7b, 0x8a, 0x20, 0xde, 0xff, 0x5f, 0x3a
};
static uint8_t bob_recv_key[] = {
    0xc1, 0x3c, 0x26, 0xb1, 0x6e, 0x0e, 0x05, 0x37,
    0xa3, 0xa3, 0x61, 0x71, 0xfe, 0x5f, 0xae, 0xb2
};
static uint8_t expected_common_iv[] = {
    0x12, 0xee, 0xc5, 0x4f, 0x8c, 0x34, 0xeb, 0x6b,
    0xa7, 0x8d, 0x2c, 0x5a, 0x56
};
static uint8_t expected_nonce[] = {
    0x12, 0xee, 0xc5, 0x4f, 0x8c, 0x34, 0x47, 0x89,
    0xa7, 0x8d, 0x2c, 0x58, 0x6a
};
static int expected_nonce_seqnr = 572;

static uint8_t expected_encoded_message[] = {
0xd0, 0x83, 0x43, 0xa1, 0x1, 0xa, 0xa1, 0x5, 0x4d, 0x12, 0xee, 0xc5, 0x4f, 0x8c, 0x34, 0x47, 0x89, 0xa7, 0x8d, 0x2c, 0x5a, 0x56, 0x58, 0x18, 0x9a, 0x21, 0xbf, 0x4a, 0xea, 0x7c, 0xfe, 0x3f, 0xc5, 0xc7, 0xac, 0xe6, 0xc5, 0x9a, 0x46, 0x53, 0x3c, 0xd8, 0xa2, 0x26, 0x8c, 0xe2, 0xdc, 0xbd
};

static uint8_t buf[256];
static uint8_t dbuf[256];
static char message[] = "a secret message";

static void setUp(void)
{
    /* ... */
}

static void tearDown(void)
{
    /* ... */
}

static void test_security_ctx_key_gen(void)
{
    uint8_t zeros[SECURITY_CTX_KEY_LEN] = { 0 };
    security_ctx_t ctx;

    security_ctx_init(&ctx, bob_id, sizeof(bob_id), alice_id, sizeof(alice_id));
    TEST_ASSERT_EQUAL_INT(0, memcmp(ctx.send_ctx_key, zeros, SECURITY_CTX_KEY_LEN));
    TEST_ASSERT_EQUAL_INT(0, memcmp(ctx.recv_ctx_key, zeros, SECURITY_CTX_KEY_LEN));
    TEST_ASSERT_EQUAL_INT(0, memcmp(ctx.common_iv, zeros, SECURITY_CTX_COMMON_IV_LEN));
    TEST_ASSERT_EQUAL_INT(0, memcmp(ctx.send_id, bob_id, sizeof(bob_id)));
    TEST_ASSERT_EQUAL_INT(0, memcmp(ctx.recv_id, alice_id, sizeof(alice_id)));
    security_ctx_key_gen(&ctx, salt, sizeof(salt), secret, sizeof(secret));
    TEST_ASSERT_EQUAL_INT(0, memcmp(ctx.send_ctx_key, bob_send_key, SECURITY_CTX_KEY_LEN));
    TEST_ASSERT_EQUAL_INT(0, memcmp(ctx.recv_ctx_key, bob_recv_key, SECURITY_CTX_KEY_LEN));
    TEST_ASSERT_EQUAL_INT(0, memcmp(ctx.common_iv, expected_common_iv, SECURITY_CTX_COMMON_IV_LEN));
}

static void test_security_ctx_gen_nonce(void)
{
    security_ctx_t ctx;
    uint8_t nonce[SECURITY_CTX_NONCE_LEN];
    security_ctx_init(&ctx, bob_id, sizeof(bob_id), alice_id, sizeof(alice_id));
    security_ctx_key_gen(&ctx, salt, sizeof(salt), secret, sizeof(secret));
    ctx.seqnr = expected_nonce_seqnr;
    security_ctx_gen_nonce(&ctx, bob_id, sizeof(bob_id), nonce);
    TEST_ASSERT_EQUAL_INT(0, memcmp(nonce, expected_nonce, SECURITY_CTX_NONCE_LEN));
    TEST_ASSERT_EQUAL_INT(ctx.seqnr, expected_nonce_seqnr + 1);
}

static void test_security_ctx_encode_decode(void)
{
    security_ctx_t alice_ctx;
    security_ctx_t bob_ctx;
    security_ctx_init(&alice_ctx, alice_id, sizeof(alice_id), bob_id, sizeof(bob_id));
    security_ctx_init(&bob_ctx, bob_id, sizeof(bob_id), alice_id, sizeof(alice_id));
    security_ctx_key_gen(&alice_ctx, salt, sizeof(salt), secret, sizeof(secret));
    security_ctx_key_gen(&bob_ctx, salt, sizeof(salt), secret, sizeof(secret));
    TEST_ASSERT_EQUAL_INT(0, memcmp(alice_ctx.recv_ctx_key, bob_ctx.send_ctx_key, SECURITY_CTX_KEY_LEN));
    TEST_ASSERT_EQUAL_INT(0, memcmp(alice_ctx.send_ctx_key, bob_ctx.recv_ctx_key, SECURITY_CTX_KEY_LEN));
    TEST_ASSERT_EQUAL_INT(0, memcmp(alice_ctx.common_iv, bob_ctx.common_iv, SECURITY_CTX_COMMON_IV_LEN));
    uint8_t *out;
    size_t olen = security_ctx_encode(&bob_ctx, (uint8_t*) message, strlen(message), buf, sizeof(buf), &out);
    TEST_ASSERT_EQUAL_INT(0, memcmp(out, expected_encoded_message, olen));
    uint8_t msg[sizeof(message)];
    size_t msg_len = 0;
    TEST_ASSERT_EQUAL_INT(0, security_ctx_decode(&alice_ctx, out, olen, dbuf, sizeof(dbuf), msg, &msg_len));
    TEST_ASSERT_EQUAL_INT(0, memcmp(message, msg, msg_len));
}

Test *tests_security_ctx_all(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_security_ctx_key_gen),
        new_TestFixture(test_security_ctx_gen_nonce),
        new_TestFixture(test_security_ctx_encode_decode),
    };

    EMB_UNIT_TESTCALLER(security_ctx_tests, setUp, tearDown, fixtures);
    return (Test *)&security_ctx_tests;
}

void tests_security_ctx(void)
{
    TESTS_RUN(tests_security_ctx_all());
}
