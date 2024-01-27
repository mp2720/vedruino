#pragma once

#include <Arduino.h>

// TODO: переделать

#define VDR_LOGE(tag, ...)                                                                         \
    {                                                                                              \
        printf("ERR [%s]: ", tag);                                                                 \
        printf(__VA_ARGS__);                                                                       \
        printf("\n");                                                                              \
    }
#define VDR_LOGW(tag, ...)                                                                         \
    {                                                                                              \
        printf("WARN [%s]: ", tag);                                                                \
        printf(__VA_ARGS__);                                                                       \
        printf("\n");                                                                              \
    }
#define VDR_LOGI(tag, ...)                                                                         \
    {                                                                                              \
        printf("INFO [%s]: ", tag);                                                                \
        printf(__VA_ARGS__);                                                                       \
        printf("\n");                                                                              \
    }
#define VDR_LOGV(tag, ...)                                                                         \
    {                                                                                              \
        printf("VERB [%s]: ", tag);                                                                \
        printf(__VA_ARGS__);                                                                       \
        printf("\n");                                                                              \
    }
#define VDR_LOGD(tag, ...)                                                                         \
    {                                                                                              \
        printf("DBG [%s]: ", tag);                                                                 \
        printf(__VA_ARGS__);                                                                       \
        printf("\n");                                                                              \
    }
