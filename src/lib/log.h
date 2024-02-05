#pragma once

#include "../conf.h"

#if CONF_LOG_COLOR_ENABLED == 0
    #define CLR_E ""
    #define CLR_W ""
    #define CLR_I ""
    #define CLR_D ""
    #define CLR_V ""
    #define CLR_T ""
#else
    #define CLR_E "\033[91m\033[49m" //красный
    #define CLR_W "\033[93m\033[49m" //оранжевый
    #define CLR_I "\033[92m\033[49m" //зелёный
    #define CLR_D "\033[96m\033[49m" //голубой
    #define CLR_V "\033[90m\033[49m" //тёмно-серый
    #define CLR_T "\033[39m\033[49m" //сброс цвета
#endif

#ifndef CONF_LOG_LEVEL
    #define LIBLOG_LEVEL 5
#else
    #define LIBLOG_LEVEL CONF_LOG_LEVEL
#endif

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#undef ESP_LOGV

#if LIBLOG_LEVEL >= 1 
    #define ESP_LOGE(tag, format, ...)  log_output(CLR_E "E [" CLR_T "%s" CLR_E "] " format CLR_T "\n", tag, ##__VA_ARGS__)
#else   
    #define ESP_LOGE(tag, format, ...)
#endif

#if LIBLOG_LEVEL >= 2 
    #define ESP_LOGW(tag, format, ...)  log_output(CLR_W "W [" CLR_T "%s" CLR_W "] " format CLR_T "\n", tag, ##__VA_ARGS__)
#else   
    #define ESP_LOGW(tag, format, ...)
#endif

#if LIBLOG_LEVEL >= 3 
    #define ESP_LOGI(tag, format, ...)  log_output(CLR_I "I [" CLR_T "%s" CLR_I "] " format CLR_T "\n", tag, ##__VA_ARGS__)
#else   
    #define ESP_LOGI(tag, format, ...)
#endif

#if LIBLOG_LEVEL >= 4 
    #define ESP_LOGD(tag, format, ...)  log_output(CLR_D "D [" CLR_T "%s" CLR_D "] " format CLR_T "\n", tag, ##__VA_ARGS__)
#else   
    #define ESP_LOGD(tag, format, ...)
#endif

#if LIBLOG_LEVEL >= 5 
    #define ESP_LOGV(tag, format, ...)  log_output(CLR_V "V [" CLR_T "%s" CLR_V "] " format CLR_T "\n", tag, ##__VA_ARGS__)
#else   
    #define ESP_LOGV(tag, format, ...)
#endif


typedef int (*printf_like_t) (const char *, ...);

//указатель на функцию вывода логов. Изначально printf(), изменить на tcp_log_printf() для отправки по сети
extern printf_like_t log_output; 
