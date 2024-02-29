#include "app.h"

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <MCP3221.h>
#include <MGS_FR403.h>
#include <SparkFun_SGP30_Arduino_Library.h>
#include <mcp3021.h>

static const char *TAG = "sensors";

extern appSensorData_t app_sensors;

static void poll_noise();
static void fire_init();
static void fire_poll();
static void axel_init();
static void axel_poll();
static void gas_init();
static void gas_poll();
static void water_init();
static void water_poll();

void app_sensors_init() {
    /* fire_init(); */
    /* axel_init(); */
    /* gas_init(); */
    /* water_init(); */
}

// датчик звука
static MCP3221 noise1(0x4e);
static MCP3221 noise2(0x4e);
static MCP3221 noise3(0x4e);
static void poll_noise() {
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

    app_sensors.noise[0] = (int)fabs(1250.f - res[0]);
    app_sensors.noise[1] = (int)fabs(1250.f - res[1]);
    app_sensors.noise[2] = (int)fabs(1250.f - res[2]);
}

#define FIRE_ADDR 0x39

static float fire_default_ir[3];

void app_sensors_calibrate_fire() {
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
            // fire_default_vis[i] += sensor_data[1] * 256.0 + sensor_data[0];
        }
    }
    pk_i2c_unlock();
    for (int i = 0; i < 3; i++) {
        fire_default_ir[i] /= num;
        // fire_default_vis[i] /= num;
    }
}

static void fire_init() {
    // PKLOGI("Start init fire");
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
    app_sensors_calibrate_fire();
}

static void fire_poll() {
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
        app_sensors.fire[i] = (sensor_data[3] * 256.0 + sensor_data[2]) / fire_default_ir[i];
    }
    pk_i2c_unlock();
}

Adafruit_MPU6050 mpu;

static void axel_init() {
    pk_i2c_lock();
    if (!mpu.begin(0x69)) {
        PKLOGE("Failed to find MPU6050 chip");
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    pk_i2c_unlock();
}

static void axel_poll() {
    sensors_event_t a, g, temp;
    pk_i2c_lock();
    mpu.getEvent(&a, &g, &temp);
    pk_i2c_unlock();
    app_sensors.axel = sqrtf(
                           a.acceleration.x * a.acceleration.x +
                           a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z
                       ) -
                       9.8;
}

static SGP30 sgp30_1;
static SGP30 sgp30_2;
static SGP30 sgp30_3;

static void gas_init() {
    pk_i2c_lock();
    pk_i2c_switch(5);
    if (sgp30_1.begin() == false) {
        PKLOGD("sgp30 err 1");
    }
    sgp30_1.initAirQuality();
    pk_i2c_switch(6);
    if (sgp30_2.begin() == false) {
        PKLOGD("sgp30 err 2");
    }
    sgp30_2.initAirQuality();
    pk_i2c_switch(7);
    if (sgp30_3.begin() == false) {
        PKLOGD("sgp30 err 3");
    }
    sgp30_3.initAirQuality();
    pk_i2c_unlock();
}

static void gas_poll() {
    pk_i2c_lock();
    pk_i2c_switch(5);
    sgp30_1.measureAirQuality();
    pk_i2c_switch(6);
    sgp30_2.measureAirQuality();
    pk_i2c_switch(7);
    sgp30_3.measureAirQuality();
    pk_i2c_unlock();

    app_sensors.gas[0] = sgp30_1.CO2;
    app_sensors.gas[1] = sgp30_2.CO2;
    app_sensors.gas[2] = sgp30_3.CO2;
}

// протечка
static MCP3021 mcp3021;

static void water_init() {
    pk_i2c_lock();
    mcp3021.begin(0x4d);
    pk_i2c_unlock();
}

static void water_poll() {
    const float air_value = 561.0;
    const float water_value = 293.0;
    const float moisture_0 = 0.0;
    const float moisture_100 = 100.0;
    pk_i2c_lock();
    float adc0 = mcp3021.readADC();
    pk_i2c_unlock();
    float h = map(adc0, air_value, water_value, moisture_0, moisture_100);
    app_sensors.water_flow = h;
}
