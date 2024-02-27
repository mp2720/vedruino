#include <Arduino.h>
#include "lib.h"
#include "lib/i2c_tools.h"
#include "lib/pk_assert.h"
#include <ESP32_Servo.h>


static int servo_pins[3] = {0, 0, 0};

static Servo servo[3];

void doors_init()
{
    for(int i = 0; i < 3; ++i)
    {
        servo[i].attach(servo_pins[i]);
    }
}

void doors_set(int door, bool state)
{
    if(state)
    {
        servo[door].write(0);
    }
    else
    {
        servo[door].write(180);
    }
}