#pragma once
#include <stdint.h>

/*
датчик линии
*/

typedef struct {
    int16_t val[19];
} ln19aData_t;

void ln19a_init();

ln19aData_t ln19a_get_raw();

ln19aData_t ln19a_postprocessing();

float ln19a_get_line(ln19aData_t data); //возвращает от -1 до 1