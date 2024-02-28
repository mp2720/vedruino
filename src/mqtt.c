#include "app.h"

#include <errno.h>
#include <stdio.h>

const char *TAG = "app.mqtt";

struct appMqttSensors app_mqtt_sensors;

// https://github.com/mp1884/nto_topics/blob/main/README.md#basectl
static void ctl_handler(PK_UNUSED char *topic, char *data, PK_UNUSED int len) {
    PKLOGI("/base/ctl got msg: '%s'", data);

    int flat_num;
    char ctl_name[8];
    int n = sscanf(data, "%d%s", &flat_num, ctl_name);
    if (n != 2) {
        PKLOGE("/base/ctl parse error with data %s: %s", data, strerror(errno));
        return;
    }

    if (strcmp(ctl_name, "pump") == 0) {
        PKLOGI("pump switch");
        // pump
    } else if (strcmp(ctl_name, "fire") == 0) {
        PKLOGI("fire %d switch", flat_num);
        // fire x3
    } else if (strcmp(ctl_name, "door") == 0) {
        PKLOGI("door %d switch", flat_num);
        // door x3
    } else if (strcmp(ctl_name, "street") == 0) {
        PKLOGI("street lamp switch");
        // street
    } else {
        PKLOGE("/base/ctl unknown ctl_name='%s'", ctl_name);
    }
}

static float rand_float() {
    return 1000.f / (float)(1 + (rand() % 1000));
}

void app_mqtt_init() {
    pkTopic_t topics[] = {{"/base/ctl", ctl_handler, 2}};
    // clang-format off
    // app_mqtt_sensors = (struct appMqttSensors){
    //     .noise = {1, 2, 3},
    //     .fire = {4.4, 5.5, 6.6},
    //     .gas = {7.7, 8.8, 9.9},
    //     .amperage = 10.10,
    //     .water_flow = 11.11
    // };
    // clang-format on

    PK_ASSERT(pk_mqtt_set_subscribed_topics(topics, PK_ARRAYSIZE(topics)));
    PKLOGI("subscribed to /base/ctl");
}

#define TRIPLE_VAL(name) (name)[0], (name)[1], (name)[2]

// https://github.com/mp1884/nto_topics/blob/main/README.md#basesensors
void app_mqtt_sensors_send() {
    // clang-format off
    app_mqtt_sensors = (struct appMqttSensors){
        .noise = {rand(), rand(), rand()},
        .fire = {rand_float(),rand_float(),rand_float()},
        .gas= {rand_float(),rand_float(),rand_float()},
        .amperage = rand_float(),
        .water_flow = rand_float()
    };
    // clang-format on

    char buf[256];
    int len = snprintf(
        buf,
        PK_ARRAYSIZE(buf),
        "noise:%d;%d;%d;"
        "fire:%f;%f;%f;"
        "gas:%f;%f;%f;"
        "amperage:%f;"
        "water_flow:%f",
        TRIPLE_VAL(app_mqtt_sensors.noise),
        TRIPLE_VAL(app_mqtt_sensors.fire),
        TRIPLE_VAL(app_mqtt_sensors.gas),
        app_mqtt_sensors.amperage,
        app_mqtt_sensors.water_flow
    );
    if (len < 0) {
        PKLOGE("/base/sensors sprintf() failed: %s", strerror(errno));
        return;
    }
    if (len >= (int)PK_ARRAYSIZE(buf)) {
        PKLOGE("/base/sensors buf is too small");
        return;
    }

    PKLOGV("sending to /base/sensors: %s", buf);

    if (len < 0) {
        PKLOGE("/base/sensors failed to sprintf(): %s", strerror(errno));
        return;
    }

    if (!pk_mqtt_publish("/base/sensors", buf, len, 0, false)) {
        PKLOGE("/base/sensors sending failed");
    }
}

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

// https://github.com/mp1884/nto_topics/blob/main/README.md#basenots
void app_mqtt_send_notification(appNotificationType_t type, int flat_num) {
    const char *type_str;
    switch (type) {
    case APP_NOT_EARTHQUAKE:
        type_str = "earthquake";
        break;
    case APP_NOT_SOUND:
        type_str = "sound";
        break;
    case APP_NOT_WATER_OVERFLOW:
        type_str = "water_overflow";
        break;
    case APP_NOT_GAS_LEAK:
        type_str = "gas_leak";
        break;
    case APP_NOT_GAS_LEAK_CALL_ROBOT:
        type_str = "gas_leak_call_robot";
        break;
    case APP_NOT_FIRE:
        type_str = "fire";
        break;
    case APP_NOT_FIRE_CALL_ROBOT:
        type_str = "fire_call_robot";
        break;
    }

    char buf[64];
    int len = snprintf(buf, PK_ARRAYSIZE(buf), "flat_num:%d;not_type:%s", flat_num, type_str);
    if (len < 0) {
        PKLOGE("/base/nots sprintf() failed: %s", strerror(errno));
        return;
    }
    if (len >= (int)PK_ARRAYSIZE(buf)) {
        PKLOGE("/base/nots buf is too small");
        return;
    }
}
