#include <inttypes.h>

#include "edhoc/keys.h"
#include "xfa.h"

XFA_USE_CONST(cred_db_entry_t, cred_db);
/**
 * @brief   CBOR-encoded authentication key
 */
static const uint8_t DW5FC2_auth_key_cbor[] = {
    0xa5, 0x01, 0x01, 0x20, 0x06, 0x21, 0x58, 0x20, 0xae, 0xb0, 0xe4, 0x4a,
    0x8b, 0x56, 0xa3, 0xce, 0xb9, 0x66, 0x86, 0xa6, 0x13, 0x53, 0x99, 0x34,
    0x56, 0xd1, 0xff, 0xf0, 0x2a, 0x6d, 0xaf, 0x43, 0xbc, 0x3d, 0xd6, 0x53,
    0x4f, 0xfe, 0x2b, 0x94, 0x23, 0x58, 0x20, 0xaf, 0xfd, 0xa6, 0x5d, 0xf8,
    0xf3, 0x88, 0xb3, 0x2f, 0xfb, 0xf0, 0xf9, 0x7e, 0x10, 0x71, 0x06, 0x53,
    0xff, 0x94, 0x33, 0xaa, 0x0d, 0xb0, 0x34, 0xc5, 0x01, 0x00, 0x11, 0x9c,
    0x1a, 0x8d, 0x4c, 0x02, 0x46, 0x44, 0x57, 0x35, 0x46, 0x43, 0x32
};

/**
 * @brief   CBOR-encoded RPK
 */
static const uint8_t DW5FC2_rpk_cbor[] = {
    0xa4, 0x01, 0x01, 0x20, 0x06, 0x21, 0x58, 0x20, 0xae, 0xb0, 0xe4, 0x4a,
    0x8b, 0x56, 0xa3, 0xce, 0xb9, 0x66, 0x86, 0xa6, 0x13, 0x53, 0x99, 0x34,
    0x56, 0xd1, 0xff, 0xf0, 0x2a, 0x6d, 0xaf, 0x43, 0xbc, 0x3d, 0xd6, 0x53,
    0x4f, 0xfe, 0x2b, 0x94, 0x02, 0x46, 0x44, 0x57, 0x35, 0x46, 0x43, 0x32
};

/**
 * @brief   CBOR-encoded RPK ID
 */
static const uint8_t DW5FC2_rpk_id_cbor[] = {
    0xa1, 0x04, 0x46, 0x44, 0x57, 0x35, 0x46, 0x43, 0x32
};

/**
 * @brief   RPK ID Value
 */
static const uint8_t DW5FC2_rpk_id_value[] = {
    0x44, 0x57, 0x35, 0x46, 0x43, 0x32
};

XFA_CONST(cred_db, 0) cred_db_entry_t _DW5FC2_db_entry = {
    .auth_key = DW5FC2_auth_key_cbor,
    .auth_key_len = sizeof(DW5FC2_auth_key_cbor),
    .id = DW5FC2_rpk_id_cbor,
    .id_len = sizeof(DW5FC2_rpk_id_cbor),
    .id_value = DW5FC2_rpk_id_value,
    .id_value_len = sizeof(DW5FC2_rpk_id_value),
    .cred = DW5FC2_rpk_cbor,
    .cred_len = sizeof(DW5FC2_rpk_cbor),
};

/** @} */
