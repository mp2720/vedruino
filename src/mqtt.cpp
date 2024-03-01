#include "app.h"

#if CONF_LIB_MQTT_ENABLED

#include <stdio.h>

static const char *TAG = "app.mqtt";

struct appMqttSensors app_mqtt_sensors;

// https://github.com/mp1884/nto_topics/blob/main/README.md#basectl
static void ctl_handler(PK_UNUSED char *topic, char *data, PK_UNUSED int len) {
    PKLOGI("/base/ctl got msg: '%s'", data);

    int flat_num;
    int state;
    char ctl_name[8];
    int n = sscanf(data, "flat_num:%d;state:%d;ctl_type:%s", &flat_num, &state, ctl_name);
    if (n != 3) {
        PKLOGE("/base/ctl parse error with data %s, n=%d", data, n);
        return;
    }

    if (strcmp(ctl_name, "pump") == 0) {
        PKLOGI("pump switch");
        app_pump_switch(state);
    } else if (strcmp(ctl_name, "fire") == 0) {
        PKLOGI("fire switch for flat %d", flat_num);
        appLedStripColor_t col = state ? APP_LED_BLUE : APP_LED_BLACK;
        app_led_strip_set_from_to(0, APP_LEDS_NUM - 1, col);
    } else if (strcmp(ctl_name, "door") == 0) {
        PKLOGI("door switch for flat %d", flat_num);
        app_door_set(flat_num, !state);
    } else if (strcmp(ctl_name, "street") == 0) {
        PKLOGI("street lamp switch");
        app_lamp_switch(state);
    } else if (strcmp(ctl_name, "sound") == 0) {
        PKLOGI("sound led switch for flat %d", flat_num);
        appLedStripColor_t col = state ? APP_LED_RED : APP_LED_BLACK;
        app_led_strip_set_flat(flat_num, col);
    } else if (strcmp(ctl_name, "gas_leak") == 0) {
        appLedStripColor_t col = state ? APP_LED_YELLOW : APP_LED_BLACK;
        app_led_strip_set_from_to(0, APP_LEDS_NUM - 1, col);
    } else {
        PKLOGE("/base/ctl unknown ctl_name='%s'", ctl_name);
    }
}

static pkTopic_t topics[] = {
    {"/base/ctl", ctl_handler, 2},
};

void app_mqtt_init() {
    PK_ASSERT(pk_mqtt_set_subscribed_topics(topics, PK_ARRAYSIZE(topics)));
    PKLOGI("subscribed to /base/ctl");
}

#define TRIPLE_VAL(name) (name)[0], (name)[1], (name)[2]

// https://github.com/mp1884/nto_topics/blob/main/README.md#basesensors
void app_mqtt_sensors_send() {
    char buf[256];
    int len = snprintf(
        buf,
        PK_ARRAYSIZE(buf),
        "noise:%d;%d;%d;"
        "fire:%f;%f;%f;"
        "gas:%f;%f;%f;"
        "axel:%f;"
        "amperage:%f;"
        "water_flow:%f;"
        "water_overflow:%f",
        TRIPLE_VAL(app_sensors.noise),
        TRIPLE_VAL(app_sensors.fire),
        TRIPLE_VAL(app_sensors.gas),
        app_sensors.axel,
        app_sensors.amperage,
        app_sensors.water_flow,
        app_sensors.water_overflow
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

// https://github.com/mp1884/nto_topics/blob/main/README.md#basenots
void app_mqtt_send_notification(appNotificationType_t type, int flat_num) {
    char buf[64];
    const char *type_str;
    const char *topic = NULL;
    int len;
    switch (type) {
    case APP_NOT_EARTHQUAKE:
        type_str = "earthquake";
        break;
    case APP_NOT_SOUND:
        type_str = "sound";
        break;
    case APP_NOT_SOUND_ADMIN:
        type_str = "sound_admin";
        break;
    case APP_NOT_WATER_OVERFLOW:
        type_str = "water_overflow";
        break;
    case APP_NOT_GAS_LEAK:
        type_str = "gas_leak";
        break;
    case APP_NOT_GAS_LEAK_CALL_ROBOT:
        type_str = "gas_leak_call_robot";
        len = sprintf(buf, "%d", flat_num);
        topic = "/robot/gas";
        break;
    case APP_NOT_FIRE:
        type_str = "fire";
        break;
    case APP_NOT_FIRE_CALL_ROBOT:
        type_str = "fire_call_robot";
        len = sprintf(buf, "%d", flat_num);
        topic = "/robot/fire";
        break;
    }

    if (topic == NULL) {
        len = snprintf(buf, PK_ARRAYSIZE(buf), "flat_num:%d;not_type:%s", flat_num, type_str);
        topic = "/base/nots";
    }

    if (len < 0) {
        PKLOGE("snprintf() failed: %s", strerror(errno));
        return;
    }
    if (len >= (int)PK_ARRAYSIZE(buf)) {
        PKLOGE("buf for snprintf() is too small");
        return;
    }
    pk_mqtt_publish(topic, buf, len, 2, false);
}

#endif // CONF_LIB_MQTT_ENABLED
