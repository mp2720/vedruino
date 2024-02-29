#include "app.h"

#include <Arduino.h>
#include <FastLED.h>

#define NUM_LEDS 17 // указываем количество светодиодов на ленте
#define PIN 25

static CRGB leds[NUM_LEDS];

static void led_strip_init() {
    FastLED.addLeds<WS2812, PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(0);
}

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

#define PUMP_RELAY_SELECT 1

static bool pump_state;

void app_pump_switch() {
    pump_state = !pump_state;
    app_relay_set(PUMP_RELAY_SELECT, pump_state);
}

void app_led_fire_set(int flat_num, bool flag) {
    appLedStripColor_t col;
    if (flag)
        col = APP_LED_BLUE;
    else
        col = APP_LED_BLACK;

    app_led_strip_set_color(flat_num, col);
}

void app_led_gas_leak_set(int flat_num, bool flag) {
    appLedStripColor_t col;
    if (flag)
        col = APP_LED_YELLOW;
    else
        col = APP_LED_BLACK;

    app_led_strip_set_color(flat_num, col);
}

void app_led_earthquake_set(bool flag) {
    appLedStripColor_t col;
    if (flag)
        col = APP_LED_YELLOW;
    else
        col = APP_LED_BLACK;

    for (int i = 0; i < 3; ++i)
        app_led_strip_set_color(i, col);
}

void app_led_sound_set(int flat_num, bool flag) {
    appLedStripColor_t col;
    if (flag)
        col = APP_LED_RED;
    else
        col = APP_LED_BLACK;

    app_led_strip_set_color(flat_num, col);
}
