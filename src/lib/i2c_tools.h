#pragma once
#include <stdint.h>

typedef enum {
    PK_SW_NONE = 0,  //нет мультиплексора
    PK_SW_PCA9547,   //смотреть на плате
    PK_SW_PW548A,    //смотреть на плате
} pk_i2c_switcher_t;

//инициализация инструментов и Wire.begin()
void pk_i2c_begin(pk_i2c_switcher_t switcher);

//заблокировать i2c для других задач, перед работой с ним
void pk_i2c_lock();

//обязательно разблокировать i2c после работы с ним
void pk_i2c_unlock();

//переключить мультиплексор на линию 3 - 7
void pk_i2c_switch(uint8_t i2c_line);

//сканирование датчиков
void pk_i2c_scan();