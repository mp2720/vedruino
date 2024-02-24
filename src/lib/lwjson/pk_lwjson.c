#include "../inc.h"

// 3 бита
#define PK_LWJSON_TYPE_MASK 7

static const char *TAG = "lwjson";

bool pk_lwjson_ret_handle(lwjsonr_t ret) {
    switch (ret) {
    case lwjsonOK:
        return true;
    case lwjsonERRPAR:
        PK_ASSERT(0);
    case lwjsonERRJSON:
        PKLOGE("malformed json");
        return false;
    case lwjsonERRMEM:
        PKLOGE("memory error");
        return false;
    case lwjsonERR:
        PKLOGE("generic error");
        return false;
    default:
        PK_ASSERT(0);
    }
}

const lwjson_token_t *pk_lwjson_findt_ex(lwjson_t *lwobj, const lwjson_token_t *token,
                                         const char *path, int type_mask) {
    const lwjson_token_t *tok = lwjson_find_ex(lwobj, token, path);
    if (tok == NULL) {
        PKLOGE("%s not found", path);
        return NULL;
    }

    if (type_mask & PK_LWJSON_TYPE_IGNORE)
        return tok;

    if (type_mask & PK_LWJSON_NULLABLE && tok->type == LWJSON_TYPE_NULL)
        return tok;

    if ((int)tok->type == (type_mask & PK_LWJSON_TYPE_MASK))
        return tok;

    if (tok->type == LWJSON_TYPE_FALSE && (type_mask & PK_LWJSON_TYPE_MASK) == PK_LWJSON_BOOL)
        return tok;

    PKLOGE("%s has invalid type", path);
    return NULL;

}
