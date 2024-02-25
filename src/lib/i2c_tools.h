#pragma once

#include "../conf.h"
#include "macro.h"
#include <stdint.h>
#include "inc.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "macro.h"

#define PK_I2C_MAX_WAIT_MS pdMS_TO_TICKS(1000)

extern SemaphoreHandle_t pk_i2c_mutex;

PK_EXTERNC_BEGIN

typedef enum {
    PK_SW_NONE = 0, // нет мультиплексора
    PK_SW_PCA9547,  // смотреть на плате
    PK_SW_PW548A,   // смотреть на плате
} pkI2cSwitcher_t;

// инициализация инструментов и Wire.begin()
void pk_i2c_begin(pkI2cSwitcher_t switcher);

//заблокировать i2c для других задач, перед работой с ним
#define pk_i2c_lock() PK_ASSERT(xSemaphoreTakeRecursive(pk_i2c_mutex, PK_I2C_MAX_WAIT_MS) == pdTRUE)

//обязательно разблокировать i2c после работы с ним
#define pk_i2c_unlock() PK_ASSERT(xSemaphoreGiveRecursive(pk_i2c_mutex) == pdTRUE)

// переключить мультиплексор на линию 3 - 7
void pk_i2c_switch(int i2c_line);

// сканирование датчиков
void pk_i2c_scan();

PK_EXTERNC_END