#pragma once

/*
датчик линии
*/

typedef struct {
    int val[19];
} ln19a_data_t;

void ln19a_init();

ln19a_data_t ln19a_get();
