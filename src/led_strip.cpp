#include "lib.h"
#include <Arduino.h>
#include <FastLED.h>

#define NUM_LEDS 17 // указываем количество светодиодов на ленте
#define PIN 25

static CRGB leds[NUM_LEDS];

void app_led_strip_init() {
    FastLED.addLeds<WS2812, PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(0);
}

void app_led_strip_set_color(int flat, appLedStripColor_t color) 
{
    FastLED.clear(true);

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
    }
    for (int i = led_min; i <= led_max; ++i)
        leds[i] = vcolor;

    FastLED.setBrightness(50);
    FastLED.show();
}
