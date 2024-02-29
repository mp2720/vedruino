#include "app.h"

#include <MGS_FR403.h>
#include <WiFi.h>
#include <cmath>
#include <freertos/FreeRTOS.h>

static const char *TAG = "main";

void setup() {
#if CONF_LIB_I2C_ENABLED
    /* pk_i2c_begin(PK_SW_PCA9547); */
#if CONF_LIB_I2C_RUN_SCANNER
    /* pk_i2c_scan(); */
#endif // CONF_LIB_I2C_RUN_SCANNER
#endif // CONF_LIB_I2C_ENABLED

    /* app_ctl_init(); */

    delay(CONF_MISC_STARTUP_DELAY);

#if CONF_LIB_WIFI_ENABLED
    WiFi.begin(CONF_LIB_WIFI_SSID, CONF_LIB_WIFI_PASSWORD);
    PKLOGI("connecting to %s...", CONF_LIB_WIFI_SSID);
    PKLOGI("board MAC: %s", WiFi.macAddress().c_str());
    while (WiFi.status() != WL_CONNECTED) {
        // PKLOGI("%d", WiFi.status());
        delay(100);
    }
    PKLOGI("WiFi connection established");
    PKLOGI("board IP: %s", WiFi.localIP().toString().c_str());
#endif // CONF_LIB_WIFI_ENABLED

#if CONF_LIB_MDNS_ENABLED
    if (!pk_mdns_init())
        PKLOGE("failed to start mdns");
#endif // CONF_LIB_MDNS_ENABLED

#if CONF_LIB_NETLOG_ENABLED
    pk_netlog_init();
#endif // CONF_LIB_NETLOG_ENABLED

#if CONF_LIB_OTA_ENABLED
    if (!ota_server_start())
        PKLOGE("failed to start ota server");
#endif // CONF_LIB_OTA_ENABLED

#if CONF_LIB_MQTT_ENABLED
    if (!pk_mqtt_connect())
        PKLOGE("failed to connect mqtt");
#endif // CONF_LIB_MQTT_ENABLED

#if CONF_LIB_MQTT_ENABLED
    app_mqtt_init();
#endif // CONF_LIB_MQTT_ENABLED

    /* app_sensors_init(); */
    app_water_flow_measure_init();

    PKLOGI("setup finished");
}

#define AXEL_THRESHOLD 0.5
#define SOUND_THRESHOLD 500
#define GAS_THRESHOLD 600
#define WATER_OVERFLOW_THRESHOLD 400
#define FIRE_THRESHOLD 4

bool cont_event_active;

// Если 0, то громкого звука не было в течении как минимум 5 секунд.
static TickType_t loud_noise_start;
static TickType_t fire_start;
static TickType_t gas_leak_start;
static TickType_t earthquake_start;

static int cont_event_flat_num;

static void check_events(TickType_t loop_start) {
    for (int i = 0; i < 3; ++i) {
        int flat_num = i + 1;

        // noise
        if (app_sensors.noise[i] > SOUND_THRESHOLD) {
            PKLOGW("loud sound start in flat %d", flat_num);
            loud_noise_start = loop_start;
            cont_event_flat_num = flat_num;
            app_mqtt_send_notification(APP_NOT_SOUND, flat_num);
            goto cont_event_occurred;
        }

        // gas
        if (app_sensors.gas[i] > SOUND_THRESHOLD) {
            PKLOGW("gas leak in flat %d", flat_num);
            gas_leak_start = loop_start;
            cont_event_flat_num = flat_num;
            app_led_strip_set_from_to(0, APP_LEDS_NUM - 1, APP_LED_YELLOW);
            app_open_all_doors();
            app_mqtt_send_notification(APP_NOT_GAS_LEAK, flat_num);
            goto cont_event_occurred;
        }

        // fire
        if (app_sensors.fire[i] > FIRE_THRESHOLD) {
            PKLOGW("fire in flat %d", flat_num);
            fire_start = loop_start;
            cont_event_flat_num = flat_num;
            app_led_strip_set_from_to(0, APP_LEDS_NUM - 1, APP_LED_BLUE);
            app_mqtt_send_notification(APP_NOT_FIRE, flat_num);
            goto cont_event_occurred;
        }
    }

    // axel
    if (abs(app_sensors.axel) > AXEL_THRESHOLD) {
        PKLOGW("earthquake start");
        app_led_strip_set_from_to(0, APP_LEDS_NUM - 1, APP_LED_YELLOW);
        app_open_all_doors();
        app_mqtt_send_notification(APP_NOT_EARTHQUAKE, 0);
        earthquake_start = loop_start;
        goto cont_event_occurred;
    }

    // water overflow
    if (app_sensors.water_overflow < WATER_OVERFLOW_THRESHOLD) {
        PKLOGW("water overflow");
        app_mqtt_send_notification(APP_NOT_WATER_OVERFLOW, 0);
        return;
    }

    return;

cont_event_occurred:
    cont_event_active = true;
    return;
}

#define TIME_CHECK(t) ((t) != 0 && (t) + pdMS_TO_TICKS(5000) < loop_start)

static void check_cont_events(TickType_t loop_start) {
    /* PKLOGD("loop_start=%d earthquake_start=%d", loop_start, earthquake_start); */
    if (TIME_CHECK(gas_leak_start)) {
        app_led_strip_set_from_to(0, APP_LEDS_NUM - 1, APP_LED_BLACK);
        PKLOGW("gas end, robot was called");
        app_mqtt_send_notification(APP_NOT_GAS_LEAK_CALL_ROBOT, cont_event_flat_num);

        cont_event_active = false;
        gas_leak_start = 0;
    } else if (TIME_CHECK(fire_start)) {
        app_led_strip_set_from_to(0, APP_LEDS_NUM - 1, APP_LED_BLACK);
        PKLOGW("fire end");
        if (app_sensors.fire[cont_event_flat_num] > FIRE_THRESHOLD) {
            PKLOGW("no robot call for fire");
        } else {
            PKLOGW("robot was called for fire");
            app_mqtt_send_notification(APP_NOT_FIRE_CALL_ROBOT, cont_event_flat_num);
        }

        cont_event_active = false;
        fire_start = 0;
    } else if (TIME_CHECK(loud_noise_start)) {
        PKLOGW("loud noise end");
        if (app_sensors.noise[cont_event_flat_num] > SOUND_THRESHOLD) {
            PKLOGW("admin notification for sound was sent");
            app_mqtt_send_notification(APP_NOT_SOUND_ADMIN, cont_event_flat_num);
        } else {
            PKLOGW("no admin notification for sound");
        }

        cont_event_active = false;
        loud_noise_start = 0;
    } else if (TIME_CHECK(earthquake_start)) {
        PKLOGW("earthquake end");
        app_led_strip_set_from_to(0, APP_LEDS_NUM - 1, APP_LED_BLACK);

        cont_event_active = false;
        earthquake_start = 0;
    }
}

#define LOOP_TICK_MS 500

void loop() {
    TickType_t loop_start = xTaskGetTickCount();

    printf("zalup\n");

    PKLOGI("%lld", app_water_flow_measure_get());

    /* app_sensors_poll(); */

    /* if (!cont_event_active) { */
    /*     check_events(loop_start); */
    /* } else { */
    /*     check_cont_events(loop_start); */
    /* } */

#if CONF_LIB_MQTT_ENABLED
    app_mqtt_sensors_send();
#endif // CONF_LIB_MQTT_ENABLED

    vTaskDelay(pdMS_TO_TICKS(LOOP_TICK_MS));
}
