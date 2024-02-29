#include "app.h"

#include <Arduino.h>
#include <ESP32Servo.h>
#include <FastLED.h>
#include <PCA9536.h>

#define NUM_LEDS 17 // указываем количество светодиодов на ленте
#define PIN 25

static void led_strip_init();
static void relay_init();
static void relay_set(int sel, bool state);
static void servo_init();

void app_ctl_init() {
    led_strip_init();
    relay_init();
    servo_init();
}

static CRGB leds[NUM_LEDS];
static void led_strip_init() {
    FastLED.addLeds<WS2812, PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(0);
}

typedef enum {
    APP_LED_BLACK,
    APP_LED_RED,
    APP_LED_YELLOW,
    APP_LED_BLUE,
    APP_LED_VIOLET
} appLedStripColor_t;

static void led_strip_set_color(int flat, appLedStripColor_t color) {
    int led_min, led_max;
    if (flat == 1) {
        led_min = 0;
        led_max = 3;
    } else if (flat == 2) {
        led_min = 6;
        led_max = 9;
    } else if (flat == 3) {
        led_min = 12;
        led_max = 15;
    } else {
        // квартиры > 3 не бывает
        PK_ASSERT(0);
    }

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
    for (int i = led_min; i <= led_max; ++i)
        leds[i] = vcolor;

    FastLED.setBrightness(50);
    FastLED.show();
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
    pk_i2c_lock();
    {
        pk_i2c_switch(5);
        if (sel == 0) {
            pca9536.setState(IO0, state ? IO_HIGH : IO_LOW);
        } else {
            pca9536.setState(IO1, state ? IO_HIGH : IO_LOW);
        }
    }
    pk_i2c_unlock();
}

#define PUMP_RELAY_SELECT 1
#define LAMP_RELAY_SELECT 0

void app_pump_switch(bool state) {
    relay_set(PUMP_RELAY_SELECT, state);
}

void app_lamp_switch(bool state) {
    relay_set(LAMP_RELAY_SELECT, state);
}

void app_led_fire_set(int flat_num, bool flag) {
    appLedStripColor_t col;
    if (flag)
        col = APP_LED_BLUE;
    else
        col = APP_LED_BLACK;

    led_strip_set_color(flat_num, col);
}

void app_led_gas_leak_set(int flat_num, bool flag) {
    appLedStripColor_t col;
    if (flag)
        col = APP_LED_YELLOW;
    else
        col = APP_LED_BLACK;

    led_strip_set_color(flat_num, col);
}

void app_led_earthquake_set(bool flag) {
    appLedStripColor_t col;
    if (flag)
        col = APP_LED_YELLOW;
    else
        col = APP_LED_BLACK;

    for (int i = 0; i < 3; ++i)
        led_strip_set_color(i, col);
}

void app_led_sound_set(int flat_num, bool flag) {
    appLedStripColor_t col;
    if (flag)
        col = APP_LED_RED;
    else
        col = APP_LED_BLACK;

    led_strip_set_color(flat_num, col);
}

Servo s;

static void servo_init() {
    PKLOGI_TAG("ctl", "servo attach %d", s.attach(19));
}

void app_servo_write(int deg) {
    s.write(deg);
}
