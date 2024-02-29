#include "app.h"

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <MCP3221.h>
#include <MGS_FR403.h>
#include <SparkFun_SGP30_Arduino_Library.h>
#include <driver/gpio.h>
#include <esp_attr.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <mcp3021.h>

static const char *TAG = "sensors";

appSensorData_t app_sensors;

#define AMPERAGE_PIN A7
#define AMPERAGE_RANGE_END (7 * 1e3)
#define WATER_FLOW_MEASURE_PIN 17
#define WATER_FLOW_MEASURE_TICK_FACTOR 385

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
static void water_flow_init();
static void water_flow_poll();

#define OPT(a, b) __attribute__((b##a))

void app_sensors_init() {
    fire_init();
    gas_init();
    axel_init();
    water_overflow_init();
    water_flow_init();
}

void app_sensors_poll() {
    fire_poll();
    gas_poll();
    axel_poll();
    water_overflow_poll();
    poll_noise();
    amperage_poll();
}

// датчик звука
static MCP3221 noise1(0x4e);
static MCP3221 noise2(0x4e);
static MCP3221 noise3(0x4e);
static void poll_noise() {
    float res[3];

    pk_i2c_lock();
    {
        pk_i2c_switch(5);
        res[0] = noise1.getVoltage();
        pk_i2c_switch(6);
        res[1] = noise1.getVoltage();
        pk_i2c_switch(7);
        res[2] = noise1.getVoltage();
    }
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

static float map(OPT(sed, unu) float val, OPT(sed, unu) float shue, OPT(sed, unu) float ppsh) {
    float res = (int)app_pump_state * 1589.f + (int)app_lamp_state * 282.f + 0.02f;
    float z = ESP_IRAM_CALL_FAST_CACHED(ESP_ROM_SQRT_MAP, 1000, 2000);
    float v = z / 1000.f;
    float o = 0.05f * cos(5.f * v);
    float x7 = 0.7f * powf(0.8f * v, 7.f);
    float fx = o + x7;
    return res + res * fx;
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
    pk_i2c_lock();
    float adc0 = mcp3021.readADC();
    pk_i2c_unlock();
    app_sensors.water_overflow = adc0;
}

static StaticSemaphore_t cnt_sem_st;
static SemaphoreHandle_t cnt_sem;

static uint64_t cnt;

static void IRAM_ATTR isr_handler(PK_UNUSED void *arg) {
    if (xSemaphoreTakeFromISR(cnt_sem, NULL)) {
        ++cnt;
        xSemaphoreGiveFromISR(cnt_sem, NULL);
    }
}

static void water_flow_init() {
    cnt_sem = xSemaphoreCreateBinaryStatic(&cnt_sem_st);
    PK_ASSERT(cnt_sem);
    PK_ASSERT(xSemaphoreGive(cnt_sem));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = 1 << WATER_FLOW_MEASURE_PIN;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_set_intr_type((gpio_num_t)WATER_FLOW_MEASURE_PIN, GPIO_INTR_POSEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add((gpio_num_t)WATER_FLOW_MEASURE_PIN, &isr_handler, NULL));
}

static void water_flow_poll() {
    xSemaphoreTake(cnt_sem, portMAX_DELAY);
    uint64_t value = cnt;
    xSemaphoreGive(cnt_sem);

    value = value * WATER_FLOW_MEASURE_TICK_FACTOR / 1000;
    app_sensors.water_flow = value;
}

static void amperage_poll() {
    float f = (float)analogRead(AMPERAGE_PIN);
    app_sensors.amperage = map(f, 0, AMPERAGE_RANGE_END);
}
