#pragma once

#include "lib.h"

PK_EXTERNC_BEGIN

struct appMqttSensors {
    int noise[3];
    float fire[3];
    float gas[3];
    float axel;
    float amperage;
    float water_flow;
    float water_overflow;
};

typedef enum appNotificationType {
    APP_NOT_EARTHQUAKE,
    // Нужен `flat_num`
    APP_NOT_SOUND,
    APP_NOT_WATER_OVERFLOW,
    // Нужен `flat_num`
    APP_NOT_GAS_LEAK,
    // Нужен `flat_num`
    APP_NOT_GAS_LEAK_CALL_ROBOT,
    // Нужен `flat_num`
    APP_NOT_FIRE,
    // Нужен `flat_num`
    APP_NOT_FIRE_CALL_ROBOT
} appNotificationType_t;

extern struct appMqttSensors app_mqtt_sensors;

void app_mqtt_init();
void app_mqtt_sensors_send();
void app_mqtt_send_notification(appNotificationType_t type, int flat_num);

PK_EXTERNC_END
