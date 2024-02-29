#pragma once

#include "lib.h"

PK_EXTERNC_BEGIN

typedef enum {
    APP_LED_BLACK,
    APP_LED_RED,
    APP_LED_YELLOW,
    APP_LED_BLUE,
    APP_LED_VIOLET
} appLedStripColor_t;

void app_led_strip_init();
void app_led_strip_set_color(int flat, appLedStripColor_t color);

PK_EXTERNC_END
