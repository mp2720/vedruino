#pragma once
#include <stdint.h>
/*
датчик света
*/

typedef struct {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
    uint16_t clear;
} apds9960_data_t;

void apds9960_init();

apds9960_data_t apds9960_get_raw();
