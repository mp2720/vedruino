#pragma once

/*
датчик линии
*/

typedef struct {
    int val[19];
} ln19a_data_t;

void ln19a_init();

ln19a_data_t ln19a_get_raw();

ln19a_data_t ln19a_postprocessing();

float ln19a_get_line(ln19a_data_t data); //возвращает от -1 до 1