#pragma once

#include "lib.h"

#include <stdint.h>

// Подключать из основной программы для всех её глобальных символов
#include "mqtt.h"
#include "sensor.h"
#include "ctl.h"

// water flow

void app_water_flow_measure_init();
void app_water_flow_measure_reset();
uint64_t app_water_flow_measure_get();
