#pragma once

#include <Arduino.h>

// TODO: переделать

#define VDR_LOGE(tag, ...)                                                                         \
    {                                                                                              \
        Serial.printf("ERR [%s]: ", tag);                                                          \
        Serial.printf(__VA_ARGS__);                                                                \
        Serial.print("\n");                                                                        \
    }
#define VDR_LOGW(tag, ...)                                                                         \
    {                                                                                              \
        Serial.printf("WARN [%s]: ", tag);                                                          \
        Serial.printf(__VA_ARGS__);                                                                \
        Serial.print("\n");                                                                        \
    }
#define VDR_LOGI(tag, ...)                                                                         \
    {                                                                                              \
        Serial.printf("INFO [%s]: ", tag);                                                          \
        Serial.printf(__VA_ARGS__);                                                                \
        Serial.print("\n");                                                                        \
    }
#define VDR_LOGV(tag, ...)                                                                         \
    {                                                                                              \
        Serial.printf("VERB [%s]: ", tag);                                                          \
        Serial.printf(__VA_ARGS__);                                                                \
        Serial.print("\n");                                                                        \
    }
#define VDR_LOGD(tag, ...)                                                                         \
    {                                                                                              \
        Serial.printf("DBG [%s]: ", tag);                                                          \
        Serial.printf(__VA_ARGS__);                                                                \
        Serial.print("\n");                                                                        \
    }
