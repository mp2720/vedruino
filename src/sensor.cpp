#include "sensor.h"
#include "MCP3221.h"
#include "lib.h"
#include "lib/i2c_tools.h"
#include "lib/pk_assert.h"
#include <MGS_FR403.h>

// датчик звука mcp3221 0x4e - 7
// датчик звука mcp3221 0x4e - 6
// датчик звука mcp3221 0x4e - 5
// дачик пламени MGS_FR403 0x39 - 7
// дачик пламени MGS_FR403 0x39 - 6
// дачик пламени MGS_FR403 0x39 - 5

static const char *TAG = "sensors";

// датчик звука
MCP3221 noise1(0x4e);
MCP3221 noise2(0x4e);
MCP3221 noise3(0x4e);
noise_t get_noise() {
    float res[3];

    pk_i2c_lock();
    pk_i2c_switch(5);
    res[0] = noise1.getVoltage();
    pk_i2c_switch(6);
    res[1] = noise1.getVoltage();
    pk_i2c_switch(7);
    res[2] = noise1.getVoltage();
    pk_i2c_unlock();

    if (res[0] == 0) {
        PKLOGE("Noise sensor on 0x4e - 7 err");
    }
    if (res[1] == 0) {
        PKLOGE("Noise sensor on 0x4e - 6 err");
    }
    if (res[2] == 0) {
        PKLOGE("Noise sensor on 0x4e - 5 err");
    }

    noise_t ret;
    ret.v[0] = (int)fabs(1250.f - res[0]);
    ret.v[1] = (int)fabs(1250.f - res[1]);
    ret.v[2] = (int)fabs(1250.f - res[2]);

    return ret;
}

#define FIRE_ADDR 0x39
void init_fire() {
    pk_i2c_lock();
    for (int i = 0; i < 3; i++) {
        pk_i2c_switch(5 + i);
        // Wire.begin();
        Wire.beginTransmission(FIRE_ADDR);
        Wire.write(0x81);       // Регистр времени интегрирования АЦП
        Wire.write(0b00111111); // 180 мс, 65535 циклов
        Wire.endTransmission();
        Wire.beginTransmission(FIRE_ADDR);
        Wire.write(0x83);       // Регистр времени ожидания
        Wire.write(0b00111111); // 180 мс
        Wire.endTransmission();
        Wire.beginTransmission(FIRE_ADDR);
        Wire.write(0x90);       // Регистр усиления
        Wire.write(0b00000000); // Усиление 1x
        Wire.endTransmission();
        Wire.beginTransmission(FIRE_ADDR);
        Wire.write(0x80); // Регистр управления питанием
        Wire.write(0b00001011); // Включение ожидания, генератора, АЦП и ALS сенсора
        Wire.endTransmission();
    }
    pk_i2c_unlock();
    file_calibrate();
}

static float fire_default_ir[3];

void file_calibrate() {
    pk_i2c_lock();
    const int num = 10;
    for (int j = 0; j < num; j++) {
        for (int i = 0; i < 3; i++) {
            pk_i2c_switch(5 + i);
            unsigned int sensor_data[4];
            Wire.beginTransmission(sensor_addr);
            Wire.write(0x94); // Начальный адрес регистров данных
            Wire.endTransmission();
            Wire.requestFrom(sensor_addr, 4);
            if (Wire.available() == 4) {
                sensor_data[0] = Wire.read();
                sensor_data[1] = Wire.read();
                sensor_data[2] = Wire.read();
                sensor_data[3] = Wire.read();
            }

            fire_default_ir[i] += sensor_data[3] * 256.0 + sensor_data[2];
            //fire_default_vis[i] += sensor_data[1] * 256.0 + sensor_data[0];
        }
        
    }
    pk_i2c_unlock();
    for (int i = 0; i < 3; i++) {
        fire_default_ir[i] /= num;
        //fire_default_vis[i] /= num;
    }
}

fire_t get_fire() {
    fire_t res;
    pk_i2c_lock();
    for (int i = 0; i < 3; i++) {
        pk_i2c_switch(5 + i);
        unsigned int sensor_data[4];
        Wire.beginTransmission(sensor_addr);
        Wire.write(0x94); // Начальный адрес регистров данных
        Wire.endTransmission();
        Wire.requestFrom(sensor_addr, 4);
        if (Wire.available() == 4) {
            sensor_data[0] = Wire.read();
            sensor_data[1] = Wire.read();
            sensor_data[2] = Wire.read();
            sensor_data[3] = Wire.read();
        }
        res.v[i] = (sensor_data[3] * 256.0 + sensor_data[2])/fire_default_ir[i];
    }
    pk_i2c_unlock();
    return res;
}