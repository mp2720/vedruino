# JSON
В `lib/` есть библиотека `lwjson`:
* [github](https://github.com/MaJerle/lwjson)
* [api](https://docs.majerle.eu/projects/lwjson/en/latest/api-reference/lwjson.html)
* [user manual](https://docs.majerle.eu/projects/lwjson/en/latest/user-manual/index.html)

`lwjson` разбирает исходный текст JSON на токены, которые формируют АСД.
Токены хранятся в статическом массиве.
Cтроки из JSON не копируются, вместо этого они хранятся в виде указателей на исходный текст JSON.
Поэтому исходный текст должен быть доступен все время, пока происходят обращения к токенам.
Также нельзя изменять исходный текст JSON, в том числе через указатели в токенах.

Получение данные из токенов проводится обходом по АСД.
Можно испоьзовать функции `lwjson_find()` и `lwjson_find_ex()`, которые находят значение в дереве
по пути.

В `lib/lwjson/pk_lwjson.h` есть некоторые вспомогательные функции.

Для генерации JSON можно использовать `snprintf()`.
Писать JSON в C строке неудобно, поэтому сделан `tools/cjsongen.py`, который генерирует C код
для массива символов, содержащий переданную в stdin строку.

`lwjson` парсит вещественные числа в `float`, а целые в `long long`.

## Пример
```c
#include "lib.h"

static const char *TAG = "json_example";

char json_text[1024];

void gen_json(void) {
    /* JSON
    {
        "obj": {
            "str_field": "example string",
            "arr": [%d, %s],
            "int_value": %d
        }
    }
    */
    const char fmt[] = {123, 34,  111, 98,  106, 34,  58,  123, 34,  115, 116, 114, 95,  102,
                        105, 101, 108, 100, 34,  58,  34,  101, 120, 97,  109, 112, 108, 101,
                        32,  115, 116, 114, 105, 110, 103, 34,  44,  34,  97,  114, 114, 34,
                        58,  91,  37,  100, 44,  37,  115, 93,  44,  34,  105, 110, 116, 95,
                        118, 97,  108, 117, 101, 34,  58,  37,  100, 125, 125, 0};

    int n = snprintf(json_text, PK_ARRAYSIZE(json_text), fmt, 1498, "true", 282);
    if (n >= (int)PK_ARRAYSIZE(json_text))
        PKLOGE("json_text is too small");
}

void parse_json(void) {
    lwjson_t lwjson;
    lwjson_token_t tokens[128];
    PK_ASSERT(lwjson_init(&lwjson, tokens, PK_ARRAYSIZE(tokens)) == lwjsonOK);

    lwjsonr_t ret = lwjson_parse(&lwjson, json_text);
    // Или
    // lwjsonr_t ret = lwjson_parse_ex(&lwjson, json_text, PK_ARRAYSIZE(json_text) - 1);
    if (!pk_lwjson_ret_handle(ret))
        return;

    const lwjson_token_t *tok = pk_lwjson_findt(&lwjson, "obj.str_field", PK_LWJSON_STRING);
    if (tok != NULL)
        // Здесь используется %.*s, потому что строки в токенах lwjson не оканчиваются нулём.
        printf("obj.str_field = %.*s\n", tok->u.str.token_value_len, tok->u.str.token_value);

    tok = pk_lwjson_findt(&lwjson, "obj.arr.#1", PK_LWJSON_BOOL);
    if (tok != NULL)
        printf("obj.arr.#1 = %s\n", tok->type == LWJSON_TYPE_TRUE ? "true" : "false");

    tok = pk_lwjson_findt(&lwjson, "obj.int_value", PK_LWJSON_REAL);
    if (tok != NULL) // false
        printf("int_value = %f", tok->u.num_real);
}
```
