#include "ln19a.h"
#include "lib.h"
#include "lib/i2c_tools.h"
#include <Wire.h>
#include <cstdint>

static const char *TAG = "ln19a";

#define SENSOR_ADDR 0x3F // Переключатели адреса в положении "OFF"



void ln19a_init() {
    pk_i2c_lock();
    Wire.beginTransmission(SENSOR_ADDR);
    Wire.write(0x10);       // Регистр настройки всей микросхемы
    Wire.write(0b00000000); // Нормальный режим работы
    Wire.write(0b01001111); // АЦП в непрерывном режиме, 200 ksps, встроенный ИОН для ЦАП
    uint8_t res = Wire.endTransmission();
    if (res != 0) {
        PKLOGE("ln19a_init() i2c error: %d", res);
    }
    delay(100);

    for (int i = 0x20; i <= 0x32; i++) {
        Wire.beginTransmission(SENSOR_ADDR);
        Wire.write(i); // Регистр настройки порта 0 (подключен к оптическому сенсору)
        Wire.write(0b00000000); // Сброс конфигурации порта
        Wire.write(0b00000000);
        res = Wire.endTransmission();
        if (res != 0) {
            PKLOGE("ln19a_init() i2c error: %d", res);
        }
    }
    delay(100);
    for (int i = 0x20; i <= 0x32; i++) {
        Wire.beginTransmission(SENSOR_ADDR);
        Wire.write(i); // Регистр настройки порта 0 (подключен к оптическому сенсору)
        Wire.write(0b01110001); // Диапазон входного напряжения 0 ... 10 В, встроенный ИОН, порт в
                                // режиме входа АЦП
        Wire.write(0b11100000); // Порт не ассоциирован с другим портом, выборок АЦП - 128
        res = Wire.endTransmission();
        if (res != 0) {
            PKLOGE("ln19a_init() i2c error: %d", res);
        }
    }
    delay(100);
    pk_i2c_unlock();
    PKLOGI("ln19a line sensor initialized");
}

