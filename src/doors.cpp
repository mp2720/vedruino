#include "lib.h"
#include <Arduino.h>
#include <ESP32_Servo.h>

static int servo_pins[3] = {18, 19, 23};

static Servo servo[3];

void app_doors_init() {
    for (int i = 0; i < 3; ++i) {
        servo[i].attach(servo_pins[i]);
    }
}

void app_doors_set(int flat_num, bool state) {
    --flat_num;
    if (state) {
        servo[flat_num].write(0);
    } else {
        servo[flat_num].write(180);
    }
}
