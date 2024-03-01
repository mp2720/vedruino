#include "app.h"

#include <Arduino.h>
#include <ESP32Servo.h>
#include <FastLED.h>
#include <PCA9536.h>

static const char *TAG = "ctl";

#define PIN 25

static void led_strip_init();
static void relay_init();
static void relay_set(int sel, bool state);
static void servo_init();

bool app_pump_state, app_lamp_state;

void app_ctl_init() {
    led_strip_init();
    relay_init();
    servo_init();
}

static CRGB leds[APP_LEDS_NUM];
static void led_strip_init() {
    for (int i = 0; i < APP_LEDS_NUM; ++i)
        leds[i] = CRGB::Black;

    FastLED.addLeds<WS2812, PIN, GRB>(leds, APP_LEDS_NUM).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(0);
    FastLED.show();
}

void app_led_strip_set_from_to(int from, int to, appLedStripColor_t color) {
    CRGB vcolor;
    switch (color) {
    case APP_LED_RED:
        vcolor = CRGB::Red;
        break;
    case APP_LED_YELLOW:
        vcolor = CRGB::Yellow;
        break;
    case APP_LED_BLUE:
        vcolor = CRGB::Blue;
        break;
    case APP_LED_VIOLET:
        vcolor = CRGB::Violet;
        break;
    case APP_LED_BLACK:
        vcolor = CRGB::Black;
        break;
    }
    for (int i = from; i <= to; ++i)
        leds[i] = vcolor;

    FastLED.setBrightness(50);
    FastLED.show();
}

void app_led_strip_set_flat(int flat, appLedStripColor_t color) {
    PKLOGV("setting led for flat %d to %d", flat, color);

    int led_min, led_max;
    if (flat == 3) {
        led_min = 0;
        led_max = 3;
    } else if (flat == 2) {
        led_min = 6;
        led_max = 9;
    } else if (flat == 1) {
        led_min = 12;
        led_max = 15;
    } else {
        // квартиры > 3 не бывает
        PK_ASSERT(0);
    }

    app_led_strip_set_from_to(led_min, led_max, color);
}

PCA9536 pca9536;
static void relay_init() {
    pk_i2c_lock();
    {
        pk_i2c_switch(5);
        pca9536.reset();
        pca9536.setMode(IO_OUTPUT);
    }
    pk_i2c_unlock();
}

static void relay_set(int sel, bool state) {
    pk_i2c_switch(5);
    if (sel == 0) {
        pca9536.setState(IO0, state ? IO_HIGH : IO_LOW);
    } else {
        pca9536.setState(IO1, state ? IO_HIGH : IO_LOW);
    }
}

#define PUMP_RELAY_SELECT 1
#define LAMP_RELAY_SELECT 0

void app_pump_switch(bool state) {
    pk_i2c_lock();
    {
        app_pump_state = state;
        relay_set(PUMP_RELAY_SELECT, !state);
    }
    pk_i2c_unlock();
}

void app_lamp_switch(bool state) {
    pk_i2c_lock();
    {
        app_lamp_state = state;
        relay_set(LAMP_RELAY_SELECT, state);
    }
    pk_i2c_unlock();
}

static const int servo_pins[3] = {5, 19, 23};
Servo s[3];

// 5 23 19
static void servo_init() {
    for (int i = 0; i < 3; ++i) {
        s[i].attach(servo_pins[i]);
    }

    s[0].write(0);
    s[1].write(0);
    s[2].write(60);
}

void app_servo_write(int flat_num, int deg) {
    --flat_num;
    /* PKLOGI("servo %d: %d", flat_num, s[flat_num].read()); */
    s[flat_num].write(deg);
}

void app_door_set(int flat_num, bool do_open) {
    int deg;
    if (do_open)
        deg = flat_num == 3 ? 60 : 0;
    else
        deg = flat_num == 3 ? 0 : 60;

    app_servo_write(flat_num, deg);
}

void app_open_all_doors() {
    for (int i = 1; i <= 3; ++i)
        app_door_set(i, false);
}
