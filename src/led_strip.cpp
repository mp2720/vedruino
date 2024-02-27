#include <Arduino.h>
#include "lib.h"
#include "lib/i2c_tools.h"
#include "lib/pk_assert.h"
#include <FastLED.h>

#define NUM_LEDS 100 // указываем количество светодиодов на ленте
#define PIN 4


static CRGB leds[NUM_LEDS];

void led_strip_init()
{
    FastLED.addLeds <WS2812, PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(0);
}

void led_strip_set(int flat, int mode)
{
    int led_min, led_max;
    if(flat == 0)
    {
        led_min = 0;
        led_max = 9;
    }
    if(flat == 1)
    {
        led_min = 10;
        led_max = 20;
    }
    if(flat == 2)
    {
        led_min = 20;
        led_max = 30;
    }

    CRGB color;
    
    if(mode == 0)
    {
        color = CRGB::Red;
    }

    if(mode == 1)
    {
        color = CRGB::Yellow;
    }

    if(mode == 2)
    {
        color = CRGB::Blue;
    }

    if(mode == 3)
    {
        color = CRGB::Violet;
    }

    for(int i = led_min; i <= led_max; ++i)
    {
        leds[i] = color;
    }
    FastLED.show();
}