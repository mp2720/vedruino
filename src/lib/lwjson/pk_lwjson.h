#pragma once

#include "lwjson.h"
#include <stdbool.h>

#define PK_LWJSON_STRING ((int)LWJSON_TYPE_STRING)
#define PK_LWJSON_INT ((int)LWJSON_TYPE_NUM_INT)
#define PK_LWJSON_REAL ((int)LWJSON_TYPE_NUM_REAL)
#define PK_LWJSON_OBJECT ((int)LWJSON_TYPE_OBJECT)
#define PK_LWJSON_ARRAY ((int)LWJSON_TYPE_ARRAY)
// В lwjson нет типа bool, но есть два отдельных типа для true и false.
#define PK_LWJSON_BOOL ((int)LWJSON_TYPE_TRUE)
// только null
#define PK_LWJSON_NULL ((int)LWJSON_TYPE_NULL)

// В lwjson токен может иметь один из 8 типов, все пронумерованы от 0 до 7.
// Значит три бита в флаге уходит на тип.
#define _PK_LWJSON_BITS_PER_TYPE 3

// Отличие от PK_LWJSON_FLAG_NULL в том, что этот флаг должен выставляться с флагами типов,
// тогда искомый токен может иметь только два типа: null или тот, что задан флагом типа.
#define PK_LWJSON_NULLABLE (1 << _PK_LWJSON_BITS_PER_TYPE)
// Остальные флаги игнорируются, искомый токен может иметь любой тип.
#define PK_LWJSON_TYPE_IGNORE (1 << (_PK_LWJSON_BITS_PER_TYPE + 1))

// Обработка кода возврата функции lwjson.
//
// Если всё в порядке, то возвращает true.
// Если ошибка в неправильном JSON или в нехватке памяти, то выводит это в лог и возвращает false.
// Если ошибка в неправильных параметрах, переданных lwjson, то выполняет PK_ASSERT(0).
// Любые коды для потокового парсинга также выполняют PK_ASSERT(0) - они нам не понадобятся.
bool pk_lwjson_ret_handle(lwjsonr_t ret);

// Поиск токена по пути path начиная с токена token.
//
// Описание формата путей:
// https://docs.majerle.eu/projects/lwjson/en/latest/user-manual/data-access.html#find-token-in-json-tree
//
// Маска типа указывает возможный тип токена.
// Возможное использование масок:
// * PK_LWJSON_FLAG_{type} - допустим только тип {type}
// * PK_LWJSON_FLAG_{type} | PK_LWJSON_NULLABLE - допустимы только типы {type} и null
// * PK_LWJSON_TYPE_IGNORE - допустим любой тип
//
// Ошибка выводится в лог.
const lwjson_token_t *pk_lwjson_findt_ex(lwjson_t *lwobj, const lwjson_token_t *token,
                                         const char *path, int type_mask);

// Поиск токена по пути path начиная с первого токена.
//
// Описание формата путей:
// https://docs.majerle.eu/projects/lwjson/en/latest/user-manual/data-access.html#find-token-in-json-tree
//
// Маска типа указывает возможный тип токена.
// Возможное использование масок:
// * PK_LWJSON_FLAG_{type} - допустим только тип {type}
// * PK_LWJSON_FLAG_{type} | PK_LWJSON_NULLABLE - допустимы только типы {type} и null
// * PK_LWJSON_TYPE_IGNORE - допустим любой тип
//
// Возвращает указатель на токен, либо NULL, если он не найден или имеет не подходящий тип.
// Ошибка выводится в лог.
static inline const lwjson_token_t *pk_lwjson_findt(lwjson_t *lwobj, const char *path,
                                                    int type_mask) {
    return pk_lwjson_findt_ex(lwobj, NULL, path, type_mask);
}
