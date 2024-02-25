// Макрос уровня логов PK_LOG_LEVEL должен быть задан перед включением этого файла через #include.
// Включать этот файл нужно только в lib/inc.h, либо если надо задать локальный уровень логов.

// Конфиг включен, но здесь из него не берутся параметры уровня логов.
#include "../conf.h"

#include "log.h"

#if CONF_LOG_PRINT_TIME
#include <esp_timer.h>
#endif // CONF_LOG_PRINT_TIME

#ifndef _PK_LOG_H
#define _PK_LOG_H

#if CONF_LOG_PRINT_FILE_LINE
#define _PKLOG_LINE ":" PK_STRINGIZE(__LINE__)
#else
#define _PKLOG_LINE ""
#endif // CONF_PKLOG_PRINT_FILE_LINE

#if CONF_LOG_PRINT_COLOR
#define _PKLOG_CLR_E "\033[91m\033[49m" // красный
#define _PKLOG_CLR_W "\033[93m\033[49m" // оранжевый
#define _PKLOG_CLR_I "\033[92m\033[49m" // зелёный
#define _PKLOG_CLR_D "\033[96m\033[49m" // голубой
#define _PKLOG_CLR_V "\033[90m\033[49m" // тёмно-серый
#define _PKLOG_CLR_R "\033[39m\033[49m" // сброс цвета
#else
#define _PKLOG_CLR_E ""
#define _PKLOG_CLR_W ""
#define _PKLOG_CLR_I ""
#define _PKLOG_CLR_D ""
#define _PKLOG_CLR_V ""
#define _PKLOG_CLR_R ""
#endif

#define _PKLOG_LETTER_E "E"
#define _PKLOG_LETTER_W "W"
#define _PKLOG_LETTER_I "I"
#define _PKLOG_LETTER_D "D"
#define _PKLOG_LETTER_V "V"

#define _PKLOG_NOP \
    do {           \
    } while (0)

// clang-format off
#if CONF_LOG_PRINT_TIME
#define _PKLOGX_ARGS(x, tag, fmt, ...)                                                              \
    _PKLOG_CLR_##x _PKLOG_LETTER_##x " " _PKLOG_CLR_R "(%d) [%s" _PKLOG_LINE "] " _PKLOG_CLR_##x fmt\
    _PKLOG_CLR_R "\n", (int)(esp_timer_get_time() / 1000), tag, ##__VA_ARGS__
#else
#define _PKLOGX_ARGS(x, tag, fmt, ...)                                                              \
    _PKLOG_CLR_##x _PKLOG_LETTER_##x " " _PKLOG_CLR_R "[%s" _PKLOG_LINE "] " _PKLOG_CLR_##x fmt     \
    _PKLOG_CLR_R "\n", tag, ##__VA_ARGS__
#endif // CONF_LOG_PRINT_TIME
// clang-format on

#define _PKLOGX_STDOUT(x, tag, fmt, ...) printf(_PKLOGX_ARGS(x, tag, fmt, ##__VA_ARGS__))
#define _PKLOGX_UART(x, tag, fmt, ...) \
    fprintf(pk_log_uartout, _PKLOGX_ARGS(x, tag, fmt, ##__VA_ARGS__))

#endif // !_PK_LOG_H

// С этого момента include-guard не действует, перед объявлением любого макроса нужно написать undef

#undef PKLOGE_TAG
#undef PKLOGE_UART_TAG

#if PKLOG_LEVEL >= 1

#if CONF_LOG_BTRACE_ON_ERROR
#define PKLOGE_TAG(tag, fmt, ...)                   \
    do {                                            \
        PK_BTRACE_MUTEX_TAKE;                       \
        _PKLOGX_STDOUT(E, tag, fmt, ##__VA_ARGS__); \
        pk_log_btrace(stdout);                      \
        PK_BTRACE_MUTEX_GIVE;                       \
    } while (0)
#define PKLOGE_UART_TAG(tag, fmt, ...)            \
    do {                                          \
        PK_BTRACE_MUTEX_TAKE;                     \
        _PKLOGX_UART(E, tag, fmt, ##__VA_ARGS__); \
        pk_log_btrace(stdout);                    \
        PK_BTRACE_MUTEX_GIVE;                     \
    } while (0)
#else
#define PKLOGE_TAG(tag, fmt, ...) _PKLOGX_STDOUT(E, tag, fmt, ##__VA_ARGS__)
#define PKLOGE_UART_TAG(tag, fmt, ...) _PKLOGX_UART(E, tag, fmt, ##__VA_ARGS__)
#endif // CONF_LOG_BTRACE_ON_ERROR

#else
#define PKLOGE_TAG(tag, fmt, ...) _PKLOG_NOP
#define PKLOGE_UART_TAG(tag, fmt, ...) _PKLOG_NOP
#endif // CONFPKLOG_LEVEL >= 1

#undef PKLOGW_TAG
#undef PKLOGW_UART_TAG

#if PKLOG_LEVEL >= 2
#define PKLOGW_TAG(tag, fmt, ...) _PKLOGX_STDOUT(W, tag, fmt, ##__VA_ARGS__)
#define PKLOGW_UART_TAG(tag, fmt, ...) _PKLOGX_UART(W, tag, fmt, ##__VA_ARGS__)
#else
#define PKLOGW_TAG(tag, fmt, ...) _PKLOG_NOP
#define PKLOGW_UART_TAG(tag, fmt, ...) _PKLOG_NOP
#endif // CONFPKLOG_LEVEL >= 2

#undef PKLOGI_TAG
#undef PKLOGI_UART_TAG

#if PKLOG_LEVEL >= 3
#define PKLOGI_TAG(tag, fmt, ...) _PKLOGX_STDOUT(I, tag, fmt, ##__VA_ARGS__)
#define PKLOGI_UART_TAG(tag, fmt, ...) _PKLOGX_UART(I, tag, fmt, ##__VA_ARGS__)
#else
#define PKLOGI_TAG(tag, fmt, ...) _PKLOG_NOP
#define PKLOGI_UART_TAG(tag, fmt, ...) _PKLOG_NOP
#endif // CONFPKLOG_LEVEL >= 3

#undef PKLOGD_TAG
#undef PKLOGD_UART_TAG

#if PKLOG_LEVEL >= 4
#define PKLOGD_TAG(tag, fmt, ...) _PKLOGX_STDOUT(D, tag, fmt, ##__VA_ARGS__)
#define PKLOGD_UART_TAG(tag, fmt, ...) _PKLOGX_UART(D, tag, fmt, ##__VA_ARGS__)
#else
#define PKLOGD_TAG(tag, fmt, ...) _PKLOG_NOP
#define PKLOGD_UART_TAG(tag, fmt, ...) _PKLOG_NOP
#endif // CONFPKLOG_LEVEL >= 4

#undef PKLOGV_TAG
#undef PKLOGV_UART_TAG

#if PKLOG_LEVEL >= 5
#define PKLOGV_TAG(tag, fmt, ...) _PKLOGX_STDOUT(V, tag, fmt, ##__VA_ARGS__)
#define PKLOGV_UART_TAG(tag, fmt, ...) _PKLOGX_UART(V, tag, fmt, ##__VA_ARGS__)
#else
#define PKLOGV_TAG(tag, fmt, ...) _PKLOG_NOP
#define PKLOGV_UART_TAG(tag, fmt, ...) _PKLOG_NOP
#endif // CONFPKLOG_LEVEL >= 5

#undef PKLOGE
#undef PKLOGW
#undef PKLOGI
#undef PKLOGD
#undef PKLOGV

#define PKLOGE(...) PKLOGE_TAG(TAG, ##__VA_ARGS__)
#define PKLOGW(...) PKLOGW_TAG(TAG, ##__VA_ARGS__)
#define PKLOGI(...) PKLOGI_TAG(TAG, ##__VA_ARGS__)
#define PKLOGD(...) PKLOGD_TAG(TAG, ##__VA_ARGS__)
#define PKLOGV(...) PKLOGV_TAG(TAG, ##__VA_ARGS__)

#undef PKLOGE_UART
#undef PKLOGW_UART
#undef PKLOGI_UART
#undef PKLOGD_UART
#undef PKLOGV_UART

#define PKLOGE_UART(...) PKLOGE_UART_TAG(TAG, ##__VA_ARGS__)
#define PKLOGW_UART(...) PKLOGW_UART_TAG(TAG, ##__VA_ARGS__)
#define PKLOGI_UART(...) PKLOGI_UART_TAG(TAG, ##__VA_ARGS__)
#define PKLOGD_UART(...) PKLOGD_UART_TAG(TAG, ##__VA_ARGS__)
#define PKLOGV_UART(...) PKLOGV_UART_TAG(TAG, ##__VA_ARGS__)
