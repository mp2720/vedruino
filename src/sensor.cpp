#include "app.h"

/* #include <ACS712.h> */
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <MCP3221.h>
#include <MGS_FR403.h>
#include <SparkFun_SGP30_Arduino_Library.h>
#include <TroykaCurrent.h>
#include <mcp3021.h>

static const char *TAG = "sensors";

appSensorData_t app_sensors;

#define AMPERAGE_PIN A7

static void poll_noise();
static void fire_init();
static void fire_poll();
static void axel_init();
static void axel_poll();
static void gas_init();
static void gas_poll();
static void water_overflow_init();
static void water_overflow_poll();
static void amperage_poll();

void app_sensors_init() {
    fire_init();
    gas_init();
    axel_init();
    water_overflow_init();
}

void app_sensors_poll() {
    fire_poll();
    gas_poll();
    axel_poll();
    water_overflow_poll();
    poll_noise();
    /* amperage_poll(); */
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
        PKLOGE("Noise sensor on 0x4e - 5 err");
    }
    if (res[1] == 0) {
        PKLOGE("Noise sensor on 0x4e - 6 err");
    }
    if (res[2] == 0) {
        PKLOGE("Noise sensor on 0x4e - 7 err");
    }

    PKLOGI("%f %f %f", res[0], res[1], res[2]);

    app_sensors.noise[0] = (int)fabs(1250.f - res[0]);
    app_sensors.noise[1] = (int)fabs(1250.f - res[1]);
    app_sensors.noise[2] = (int)fabs(1250.f - res[2]);
}

#define FIRE_ADDR 0x39

static float fire_default_ir[3];

static void fire_calibrate_one(int i) {
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
}

#define FIRE_CALIBRATE_NUM 10

void app_sensors_calibrate_fire() {
    pk_i2c_lock();
    {
        for (int i = 0; i < FIRE_CALIBRATE_NUM; ++i) {
            pk_i2c_switch(5);
            fire_calibrate_one(0);
            pk_i2c_switch(6);
            fire_calibrate_one(1);
            pk_i2c_switch(7);
            fire_calibrate_one(2);
        }
    }
    pk_i2c_unlock();

    for (int i = 0; i < 3; i++)
        fire_default_ir[i] /= FIRE_CALIBRATE_NUM;
}

static void fire_init_one() {
    /* fire.begin(); */
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

static void fire_init() {
    pk_i2c_lock();
    {
        pk_i2c_switch(5);
        fire_init_one();
        pk_i2c_switch(6);
        fire_init_one();
        pk_i2c_switch(7);
        fire_init_one();
    }
    pk_i2c_unlock();

    app_sensors_calibrate_fire();
}

static void fire_poll_one(int i) {
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
    float v = (sensor_data[3] * 256.0 + sensor_data[2]) / fire_default_ir[i];
    if (v == INFINITY || v == NAN)
        v = 0;
    app_sensors.fire[i] = v;
    /* PKLOGI("2fire %d %d %d %d", sensor_data[0], sensor_data[1], sensor_data[2], sensor_data[3]); */
    /* PKLOGI("fire %f %f", fire.ir_data, fire.vis_data); */
}

static void fire_poll() {
    pk_i2c_lock();
    {
        pk_i2c_switch(5);
        fire_poll_one(0);
        pk_i2c_switch(6);
        fire_poll_one(1);
        pk_i2c_switch(7);
        fire_poll_one(2);
    }
    pk_i2c_unlock();

    /* for (int i = 0; i < 3; i++) { */
    /*     pk_i2c_switch(5 + i); */
    /*     unsigned int sensor_data[4]; */
    /*     Wire.beginTransmission(sensor_addr); */
    /*     Wire.write(0x94); // Начальный адрес регистров данных */
    /*     Wire.endTransmission(); */
    /*     Wire.requestFrom(sensor_addr, 4); */
    /*     if (Wire.available() == 4) { */
    /*         sensor_data[0] = Wire.read(); */
    /*         sensor_data[1] = Wire.read(); */
    /*         sensor_data[2] = Wire.read(); */
    /*         sensor_data[3] = Wire.read(); */
    /*     } */
    /*     PKLOGI("fire %d: %f", i, (double)app_sensors.fire[i]); */
    /* } */
}

Adafruit_MPU6050 mpu;

static void axel_init() {
    pk_i2c_lock();
    {
        pk_i2c_switch(5);
        if (!mpu.begin(0x69)) {
            PKLOGE("Failed to find MPU6050 chip");
        }
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }
    pk_i2c_unlock();
}

static void axel_poll() {
    sensors_event_t a, g, temp;
    pk_i2c_lock();
    {
        pk_i2c_switch(5);
        if (!mpu.getEvent(&a, &g, &temp))
            PKLOGE("axel error");
    }
    pk_i2c_unlock();
    app_sensors.axel = sqrtf(
                           a.acceleration.x * a.acceleration.x +
                           a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z
                       ) -
                       9.8;
}

static SGP30 sgp30_0;
static SGP30 sgp30_1;
static SGP30 sgp30_2;

static void gas_init() {
    pk_i2c_lock();
    {
        pk_i2c_switch(5);
        if (sgp30_0.begin() == false)
            PKLOGE("sgp30 err 0");
        sgp30_0.initAirQuality();
        pk_i2c_switch(6);
        if (sgp30_1.begin() == false)
            PKLOGE("sgp30 err 1");
        sgp30_1.initAirQuality();
        pk_i2c_switch(7);
        if (sgp30_2.begin() == false)
            PKLOGE("sgp30 err 2");
        sgp30_2.initAirQuality();
    }
    pk_i2c_unlock();
}

static void handle_sgp30err(int num, SGP30ERR err) {
    switch (err) {
    case SGP30_SUCCESS:
        break;
    case SGP30_ERR_BAD_CRC:
        PKLOGE("sgp30 %d bad crc", num);
        break;
    case SGP30_ERR_I2C_TIMEOUT:
        PKLOGE("sgp30 %d i2c timeout", num);
        break;
    case SGP30_SELF_TEST_FAIL:
        PKLOGE("sgp30 %d i2c timeout", num);
        break;
    }
}

static void gas_poll() {
    SGP30ERR err;
    pk_i2c_lock();
    {
        pk_i2c_switch(5);
        err = sgp30_0.measureAirQuality();
        handle_sgp30err(0, err);
        pk_i2c_switch(6);
        err = sgp30_1.measureAirQuality();
        handle_sgp30err(1, err);
        pk_i2c_switch(7);
        err = sgp30_2.measureAirQuality();
        handle_sgp30err(2, err);
    }
    pk_i2c_unlock();

    app_sensors.gas[0] = sgp30_0.CO2;
    app_sensors.gas[1] = sgp30_1.CO2;
    app_sensors.gas[2] = sgp30_2.CO2;
}

// протечка
static MCP3021 mcp3021;

static void water_overflow_init() {
    pk_i2c_lock();
    { mcp3021.begin(0x4d); }
    pk_i2c_unlock();
}

static void water_overflow_poll() {
    /* const float air_value = 561.0; */
    /* const float water_value = 293.0; */
    /* const float moisture_0 = 0.0; */
    /* const float moisture_100 = 100.0; */
    pk_i2c_lock();
    float adc0 = mcp3021.readADC();
    pk_i2c_unlock();
    /* float h = map(adc0, air_value, water_value, moisture_0, moisture_100); */
    app_sensors.water_overflow = adc0;
}

/* ACS712 ACS(AMPERAGE_PIN, 5, 4095, 300); */
/* ACS712 sensorCurrent(AMPERAGE_PIN); */

// 282 mA
/* static void amperage_poll() { */
/*     app_sensors.amperage = (float)analogRead(AMPERAGE_PIN); */
/*     if (app_sensors.amperage > 1580) { */
/*         PKLOGI("280"); */
/*     } else { */

/*         PKLOGI("0"); */
/*     } */
/*     /1* app_sensors.amperage = ACS.mA_DC(1); *1/ */
/*     /1* app_sensors.amperage = sensorCurrent.readCurrentDC(); *1/ */

/*     PKLOGI("%f", app_sensors.amperage); */
/* } */
