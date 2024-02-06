#pragma once

#include "../conf.h"
#include "macro.h"

#if CONF_LOG_COLOR_ENABLED == 0
#define CLR_E ""
#define CLR_W ""
#define CLR_I ""
#define CLR_D ""
#define CLR_V ""
#define CLR_T ""
#else
#define CLR_E "\033[91m\033[49m" // красный
#define CLR_W "\033[93m\033[49m" // оранжевый
#define CLR_I "\033[92m\033[49m" // зелёный
#define CLR_D "\033[96m\033[49m" // голубой
#define CLR_V "\033[90m\033[49m" // тёмно-серый
#define CLR_T "\033[39m\033[49m" // сброс цвета
#endif

#ifndef CONF_LOG_LEVEL
#define LIBLOG_LEVEL 5
#else
#define LIBLOG_LEVEL CONF_LOG_LEVEL
#endif

#undef DFLT_LOGE
#undef DFLT_LOGW
#undef DFLT_LOGI
#undef DFLT_LOGD
#undef DFLT_LOGV
#undef SAFE_LOGE
#undef SAFE_LOGW
#undef SAFE_LOGI
#undef SAFE_LOGD
#undef SAFE_LOGV

#if LIBLOG_LEVEL >= 1
#define DFLT_LOGE(tag, format, ...)                                                                 \
    log_output(CLR_E "E [" CLR_T "%s" CLR_E "] " format CLR_T "\n", tag, ##__VA_ARGS__)
#else
#define DFLT_LOGE(tag, format, ...)
#endif

#if LIBLOG_LEVEL >= 2
#define DFLT_LOGW(tag, format, ...)                                                                 \
    log_output(CLR_W "W [" CLR_T "%s" CLR_W "] " format CLR_T "\n", tag, ##__VA_ARGS__)
#else
#define DFLT_LOGW(tag, format, ...)
#endif

#if LIBLOG_LEVEL >= 3
#define DFLT_LOGI(tag, format, ...)                                                                 \
    log_output(CLR_I "I [" CLR_T "%s" CLR_I "] " format CLR_T "\n", tag, ##__VA_ARGS__)
#else
#define DFLT_LOGI(tag, format, ...)
#endif

#if LIBLOG_LEVEL >= 4
#define DFLT_LOGD(tag, format, ...)                                                                 \
    log_output(CLR_D "D [" CLR_T "%s" CLR_D "] " format CLR_T "\n", tag, ##__VA_ARGS__)
#else
#define DFLT_LOGD(tag, format, ...)
#endif

#if LIBLOG_LEVEL >= 5
#define DFLT_LOGV(tag, format, ...)                                                                 \
    log_output(CLR_V "V [" CLR_T "%s" CLR_V "] " format CLR_T "\n", tag, ##__VA_ARGS__)
#else
#define DFLT_LOGV(tag, format, ...)
#endif

#if LIBLOG_LEVEL >= 1
#define SAFE_LOGE(tag, format, ...)                                                                \
    printf(CLR_E "E [" CLR_T "%s" CLR_E "] (safe) " format CLR_T "\n", tag, ##__VA_ARGS__)
#else
#define SAFE_LOGE(tag, format, ...)
#endif

#if LIBLOG_LEVEL >= 2
#define SAFE_LOGW(tag, format, ...)                                                                \
    printf(CLR_W "W [" CLR_T "%s" CLR_W "] (safe) " format CLR_T "\n", tag, ##__VA_ARGS__)
#else
#define SAFE_LOGW(tag, format, ...)
#endif

#if LIBLOG_LEVEL >= 3
#define SAFE_LOGI(tag, format, ...)                                                                \
    printf(CLR_I "I [" CLR_T "%s" CLR_I "] (safe) " format CLR_T "\n", tag, ##__VA_ARGS__)
#else
#define SAFE_LOGEI(tag, format, ...)
#endif

#if LIBLOG_LEVEL >= 4
#define SAFE_LOGD(tag, format, ...)                                                                \
    printf(CLR_D "D [" CLR_T "%s" CLR_D "] (safe) " format CLR_T "\n", tag, ##__VA_ARGS__)
#else
#define SAFE_LOGD(tag, format, ...)
#endif

#if LIBLOG_LEVEL >= 5
#define SAFE_LOGV(tag, format, ...)                                                                \
    printf(CLR_V "V [" CLR_T "%s" CLR_V "] (safe) " format CLR_T "\n", tag, ##__VA_ARGS__)
#else
#define SAFE_LOGV(tag, format, ...)
#endif

typedef int (*printf_like_t)(const char *, ...);

// указатель на функцию вывода логов. Изначально printf(), изменить на tcp_log_printf() для отправки
// по сети
EXTERNC printf_like_t log_output;
