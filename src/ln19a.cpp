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

    pk_i2c_unlock();
    delay(100);
    pk_i2c_lock();

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
    pk_i2c_unlock();
    delay(100);
    pk_i2c_lock();
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
    pk_i2c_unlock();
    delay(100);
    PKLOGI("ln19a line sensor initialized");
}

ln19a_data_t ln19a_get() {
    int adc_sensor_data[38] = {0};
    pk_i2c_lock();

    Wire.beginTransmission(SENSOR_ADDR);
    Wire.write(0x40); // Регистр данных АЦП
    Wire.endTransmission();

    Wire.requestFrom(SENSOR_ADDR, 10);
    if (Wire.available() == 10) {
        for (int i = 0; i < 10; i++) {
            adc_sensor_data[i] = Wire.read();
        }
    }
    Wire.beginTransmission(SENSOR_ADDR);
    Wire.write(0x45); // Регистр данных АЦП
    Wire.endTransmission();

    Wire.requestFrom(SENSOR_ADDR, 10);
    if (Wire.available() == 10) {
        for (int i = 10; i < 20; i++) {
            adc_sensor_data[i] = Wire.read();
        }
    }
    Wire.beginTransmission(SENSOR_ADDR);
    Wire.write(0x4A); // Регистр данных АЦП
    Wire.endTransmission();
    Wire.requestFrom(SENSOR_ADDR, 10);
    if (Wire.available() == 10) {
        for (int i = 20; i < 30; i++) {
            adc_sensor_data[i] = Wire.read();
        }
    }
    Wire.beginTransmission(SENSOR_ADDR);
    Wire.write(0x4F); // Регистр данных АЦП
    Wire.endTransmission();
    Wire.requestFrom(SENSOR_ADDR, 8);
    if (Wire.available() == 8) {
        for (int i = 30; i < 38; i++) {
            adc_sensor_data[i] = Wire.read();
        }
    }
    pk_i2c_unlock();

    ln19a_data_t result;
    for (int i = 0; i <= 19; i++) {
        result.val[i] = (adc_sensor_data[36-2*i] << 8) + adc_sensor_data[37-2*i];
    }
    return result;
}
//00000001