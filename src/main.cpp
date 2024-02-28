#include "app.h"
#include "lib/i2c_tools.h"
#include "sensor.h"
#include <MGS_FR403.h>
#include <WiFi.h>

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
       //

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

#if CONF_LIB_I2C_ENABLED
    pk_i2c_begin(PK_SW_PCA9547);
#if CONF_LIB_I2C_RUN_SCANNER
    pk_i2c_scan();
#endif // CONF_LIB_I2C_RUN_SCANNER
#endif // CONF_LIB_I2C_ENABLED
    PKLOGI("start init_sensors");

    /* app_relay_init(); */
    app_mqtt_init();

    app_led_strip_init();

    /* init_sensors(); */
    PKLOGI("setup finished");
}

void loop() {
    /* app_relay_test(); */

    /* app_mqtt_sensors.noise[1] = 1488; */
    /* app_mqtt_sensors_send(); */
    /* delay(5000); */
    /* app_mqtt_send_notification(APP_NOT_SOUND, 1); */

    app_led_strip_set_color(1, APP_LED_RED);
    delay(1000);
    app_led_strip_set_color(2, APP_LED_YELLOW);
    delay(1000);
    app_led_strip_set_color(3, APP_LED_VIOLET);
    delay(1000);

    /* app_relay_set(0, 1); */
    /* delay(2000); */
    /* app_relay_set(0, 0); */
    /* delay(2000); */
    /* app_relay_set(1, 1); */
    /* delay(2000); */
    /* app_relay_set(1, 0); */
    delay(5000);
}
