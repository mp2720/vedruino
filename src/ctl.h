#pragma once

#include "lib.h"

void app_ctl_init();

void app_pump_switch(bool state);
void app_lamp_switch(bool state);

void app_led_fire_set(int flat_num, bool flag);
void app_led_gas_leak_set(int flat_num, bool flag);
void app_led_earthquake_set(bool flag);
void app_led_sound_set(int flat_num, bool flag);

void app_servo_write(int deg);
