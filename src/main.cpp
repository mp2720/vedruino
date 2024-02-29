#include "app.h"

#include <MGS_FR403.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>

static const char *TAG = "main";

void setup() {
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

/* #if CONF_LIB_I2C_ENABLED */
/*     pk_i2c_begin(PK_SW_PCA9547); */
/* #if CONF_LIB_I2C_RUN_SCANNER */
/*     pk_i2c_scan(); */
/* #endif // CONF_LIB_I2C_RUN_SCANNER */
/* #endif // CONF_LIB_I2C_ENABLED */
    PKLOGI("start init_sensors");

#if CONF_LIB_MQTT_ENABLED
    app_mqtt_init();
#endif // CONF_LIB_MQTT_ENABLED

    /* app_led_strip_init(); */
    /* app_doors_init(); */
    /* app_sensors_init(); */

    /* init_sensors(); */
    PKLOGI("setup finished");
}

// Если 0, то громкого звука не было в течении как минимум 5 секунд.
static TickType_t loud_sound_start;
static int loud_sound_flat_num;

void loop() {
    PKLOGI("loop started");
    /* TickType_t loop_start = xTaskGetTickCount(); */

    /* if (loud_sound_start != 0 && loop_start - loud_sound_start > pdMS_TO_TICKS(5000)) { */
    /*     app_mqtt_send_notification(APP_NOT_SOUND, loud_sound_flat_num); */
    /* } */

    /* app_sensors_poll(); */

    /* for (int i = 0; i < 3; ++i) { */
    /*     app_sensors.noise[3]; */
    /* } */

/* #if CONF_LIB_MQTT_ENABLED */
/*     app_mqtt_sensors_send(); */
/* #endif // CONF_LIB_MQTT_ENABLED */

    /* delay(5000); */
}
