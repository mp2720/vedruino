#include "motors.h"
#include "lib.h"
#include "math.h"
#include <Adafruit_PWMServoDriver.h>

#define MOTORS_ADDR 0x70

static Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(MOTORS_ADDR);
static bool invert_r = false;
static bool invert_l = false;

void motors_init() {
    pk_i2c_lock();
    pwm.begin();
    // Частота (Гц)
    pwm.setPWMFreq(100);
    // Все порты выключены
    pwm.setPWM(8, 0, 4096);
    pwm.setPWM(9, 0, 4096);
    pwm.setPWM(10, 0, 4096);
    pwm.setPWM(11, 0, 4096);
    pk_i2c_unlock();
}

void motor_set_r(float pwr) {
    // Проверка, инвертирован ли мотор
    if (invert_r) {
        pwr = -pwr;
    }
    // Проверка диапазонов
    if (pwr < -100) {
        pwr = -100;
    }
    if (pwr > 100) {
        pwr = 100;
    }

    int pwmvalue = fabs(pwr) * 40.95;

    pk_i2c_lock();
    if (pwr < 0) {
        pwm.setPWM(10, 0, 4096);
        pwm.setPWM(11, 0, pwmvalue);
    } else {
        pwm.setPWM(11, 0, 4096);
        pwm.setPWM(10, 0, pwmvalue);
    }
    pk_i2c_unlock();
}

void motor_set_l(float pwr) {
    // Проверка, инвертирован ли мотор
    if (invert_r) {
        pwr = -pwr;
    }
    // Проверка диапазонов
    if (pwr < -100) {
        pwr = -100;
    }
    if (pwr > 100) {
        pwr = 100;
    }

    int pwmvalue = fabs(pwr) * 40.95;
    pk_i2c_lock();
    if (pwr < 0) {
        pwm.setPWM(8, 0, 4096);
        pwm.setPWM(9, 0, pwmvalue);
    } else {
        pwm.setPWM(9, 0, 4096);
        pwm.setPWM(8, 0, pwmvalue);
    }
    pk_i2c_unlock();
}
