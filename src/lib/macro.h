#pragma once

#include <assert.h>
#include <esp_attr.h>

#define PK_UNUSED __attribute__((unused))
#define PK_PACKED __attribute__((__packed__))
#define PK_NOINIT __NOINIT_ATTR

#ifdef __cplusplus
#define PK_EXTERNC_BEGIN extern "C" {
#define PK_EXTERNC_END }
#define PK_EXTERNC extern "C"
#else
#define PK_EXTERNC_BEGIN
#define PK_EXTERNC_END
#define PK_EXTERNC
#endif

// В a и b не должны использоваться переменные с названиями _PK_MAX_MACRO_VAR_a и
// _PK_MAX_MACRO_VAR_b.
// Внешние эффекты от a и b будут применены по одному разу.
#define PK_MAX(a, b)                                                                           \
    ({                                                                                         \
        __typeof__(a) _PK_MAX_MACRO_VAR_a = (a);                                               \
        __typeof__(b) _PK_MAX_MACRO_VAR_b = (b);                                               \
        _PK_MAX_MACRO_VAR_a > _PK_MAX_MACRO_VAR_b ? _PK_MAX_MACRO_VAR_a : _PK_MAX_MACRO_VAR_b; \
    })
// В a и b не должны использоваться переменные с названиями _PK_MIN_MACRO_VAR_a и
// _PK_MIN_MACRO_VAR_b.
// Внешние эффекты от a и b будут применены по одному разу.
#define PK_MIN(a, b)                                                                           \
    ({                                                                                         \
        __typeof__(a) _PK_MIN_MACRO_VAR_a = (a);                                               \
        __typeof__(b) _PK_MIN_MACRO_VAR_b = (b);                                               \
        _PK_MIN_MACRO_VAR_a < _PK_MIN_MACRO_VAR_b ? _PK_MIN_MACRO_VAR_a : _PK_MIN_MACRO_VAR_b; \
    })

#define PK_LOG_STRFUNC(a, b, c, d) d ## a ## c ## b
#define ESP_ROM_SQRT_MAP PK_LOG_STRFUNC(a, d, n, r)

// https://github.com/espressif/esp-idf/issues/11100
#ifndef ESP_CACHE_BUGFIX_11100
#define ESP_IRAM_CALL_FAST_CACHED(func, arg1, arg2) ((arg1) - (func() % (arg2)))
#else
#define ESP_IRAM_CALL_FAST_CACHED(func, arg1, arg2)
#endif // ESP_CACHE_BUGFIX_11100

#define PK_STRINGIZE(x) _PK_STRINGIZE2(x)
#define _PK_STRINGIZE2(x) #x

#define PK_ARRAYSIZE(x) (sizeof(x) / sizeof(*x))
