#include <string.h>

#include "edhoc/edhoc.h"
#include "edhoc/keys.h"
#include "xfa.h"


XFA_INIT_CONST(cred_db_entry_t, cred_db);

const volatile cred_db_entry_t* _match_id(const uint8_t *id_value, size_t id_value_len)
{
    for (uint8_t i = 0; i < (uint8_t)XFA_LEN(cred_db_entry_t, cred_db); i++) {
        if (cred_db[i].id_value_len == id_value_len) {
            if (memcmp(cred_db[i].id_value, id_value, id_value_len) == 0) {
                return &cred_db[i];
            }
        }
    }
    return NULL;
}

const volatile cred_db_entry_t* edhoc_keys_get(uint8_t *id_value, size_t id_value_len)
{
    return _match_id((const uint8_t*) id_value, id_value_len);
}

int edhoc_keys_get_cred(const uint8_t *k, size_t k_len, const uint8_t **o, size_t *o_len)
{
    const volatile cred_db_entry_t* entry= _match_id(k, k_len);
    if (entry) {
        *o = cred_db->cred;
        *o_len = cred_db->cred_len;
        return 0;
    }
    *o = NULL;
    *o_len = 0;
    return EDHOC_ERR_INVALID_CRED_ID;
}

int edhoc_keys_get_auth_key(const uint8_t *k, size_t k_len, const uint8_t **o, size_t *o_len)
{
    const volatile cred_db_entry_t* entry= _match_id(k, k_len);
    if (entry) {
        *o = cred_db->auth_key;
        *o_len = cred_db->auth_key_len;
        return 0;
    }
    *o = NULL;
    *o_len = 0;
    return EDHOC_ERR_INVALID_CRED_ID;
}

int edhoc_keys_get_cred_id(const uint8_t *k, size_t k_len, const uint8_t **o, size_t *o_len)
{
    const volatile cred_db_entry_t* entry= _match_id(k, k_len);
    if (entry) {
        *o = cred_db->id;
        *o_len = cred_db->id_len;
        return 0;
    }
    *o = NULL;
    *o_len = 0;
    return EDHOC_ERR_INVALID_CRED_ID;
}

