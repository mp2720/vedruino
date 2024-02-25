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
} apds9960Data_t;

void apds9960_init();

apds9960Data_t apds9960_get_raw();

typedef enum {
    //реализовать
} apds9960Color_t;

apds9960Color_t apds9960_get_color(apds9960Data_t data);