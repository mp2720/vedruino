#pragma once

#include "lib.h"

extern bool app_pump_state, app_lamp_state;

void app_ctl_init();

void app_pump_switch(bool state);
void app_lamp_switch(bool state);

#define APP_LEDS_NUM 17

typedef enum {
    APP_LED_BLACK,
    APP_LED_RED,
    APP_LED_YELLOW,
    APP_LED_BLUE,
    APP_LED_VIOLET
} appLedStripColor_t;

void app_led_strip_set_from_to(int from, int to, appLedStripColor_t color);
void app_led_strip_set_flat(int flat, appLedStripColor_t color);

void app_servo_write(int flat_num, int deg);
void app_door_set(int flat_num, bool do_open);
void app_open_all_doors();
