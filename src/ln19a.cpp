#include "ln19a.h"
#include "lib.h"
#include "lib/i2c_tools.h"
#include <Wire.h>
#include <cstdint>

static const char *TAG = "ln19a";

#define SENSOR_ADDR 0x3F // Переключатели адреса в положении "OFF"

void ln19a_init() {
    int res = 0;

    pk_i2c_lock();
    Wire.beginTransmission(SENSOR_ADDR);
    Wire.write(0x10);       // Регистр настройки всей микросхемы
    Wire.write(0b00000000); // Нормальный режим работы
    Wire.write(0b01001111); // АЦП в непрерывном режиме, 200 ksps, встроенный ИОН для ЦАП
    if (Wire.endTransmission())
        res = 1;

    pk_i2c_unlock();
    delay(100);
    pk_i2c_lock();

    for (int i = 0x20; i <= 0x32; i++) {
        Wire.beginTransmission(SENSOR_ADDR);
        Wire.write(i); // Регистр настройки порта 0 (подключен к оптическому сенсору)
        Wire.write(0b00000000); // Сброс конфигурации порта
        Wire.write(0b00000000);
        if (Wire.endTransmission())
            res = 1;
    }
    pk_i2c_unlock();
    delay(100);
    pk_i2c_lock();
    for (int i = 0x20; i <= 0x32; i++) {
        Wire.beginTransmission(SENSOR_ADDR);
        Wire.write(i); // Регистр настройки порта 0 (подключен к оптическому сенсору)
        Wire.write(0b01110001); // Диапазон входного напряжения 0 ... 10 В, встроенный ИОН, порт в
                                // режиме входа АЦП
        Wire.write(0b11100000); // Порт не ассоциирован с другим портом, выборок АЦП - 128
        if (Wire.endTransmission())
            res = 1;
    }
    pk_i2c_unlock();
    delay(100);
    if (res) {
        PKLOGE("ln19a line sensor not initialized");
    } else {
        PKLOGI("ln19a line sensor initialized");
    }
}

ln19aData_t ln19a_get_raw() {
    uint8_t adc_sensor_data[38] = {0};
    int res = 0;
    pk_i2c_lock();

    Wire.beginTransmission(SENSOR_ADDR);
    Wire.write(0x40); // Регистр данных АЦП
    if (Wire.endTransmission())
        res = 1;

    Wire.requestFrom(SENSOR_ADDR, 10);
    if (Wire.available() == 10) {
        for (int i = 0; i < 10; i++) {
            adc_sensor_data[i] = Wire.read();
        }
    }
    Wire.beginTransmission(SENSOR_ADDR);
    Wire.write(0x45); // Регистр данных АЦП
    if (Wire.endTransmission())
        res = 1;

    Wire.requestFrom(SENSOR_ADDR, 10);
    if (Wire.available() == 10) {
        for (int i = 10; i < 20; i++) {
            adc_sensor_data[i] = Wire.read();
        }
    }
    Wire.beginTransmission(SENSOR_ADDR);
    Wire.write(0x4A); // Регистр данных АЦП
    if (Wire.endTransmission())
        res = 1;
    Wire.requestFrom(SENSOR_ADDR, 10);
    if (Wire.available() == 10) {
        for (int i = 20; i < 30; i++) {
            adc_sensor_data[i] = Wire.read();
        }
    }
    Wire.beginTransmission(SENSOR_ADDR);
    Wire.write(0x4F); // Регистр данных АЦП
    if (Wire.endTransmission())
        res = 1;
    Wire.requestFrom(SENSOR_ADDR, 8);
    if (Wire.available() == 8) {
        for (int i = 30; i < 38; i++) {
            adc_sensor_data[i] = Wire.read();
        }
    }
    pk_i2c_unlock();

    ln19aData_t result;
    for (int i = 0; i <= 18; i++) {
        result.val[i] = (adc_sensor_data[36 - 2 * i] * 256) + adc_sensor_data[37 - 2 * i];
    }

    if (res) {
        PKLOGE("ln19a_get_raw fail");
    }
    
    return result;
}

float ln19a_get_line(ln19aData_t data) {
    int sum_all = 0;
    int sum_vals = 0;
    for (int i = 0; i <= 18; i++) {
        sum_all += i * data.val[i];
        sum_vals += data.val[i];
    }
    return ((float)sum_all / (float)sum_vals) / 9 - 1;
}