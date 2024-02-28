#pragma once

#include "lib.h"

PK_EXTERNC_BEGIN

struct appMqttSensors {
    int noise[3];
    float fire[3];
    float gas[3];
    float amperage;
    float water_flow;
};

extern struct appMqttSensors app_mqtt_sensors;

void app_mqtt_init();
void app_mqtt_sensors_send();

PK_EXTERNC_END
