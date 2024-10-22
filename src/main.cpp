#include "app.h"

#include <WiFi.h>

static const char *TAG = "main";

void setup() {
    delay(CONF_MISC_STARTUP_DELAY);

    WiFi.begin(CONF_LIB_WIFI_SSID, CONF_LIB_WIFI_PASSWORD);
    PKLOGI("connecting to %s...", CONF_LIB_WIFI_SSID);
    PKLOGI("board MAC: %s", WiFi.macAddress().c_str());
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }
    PKLOGI("WiFi connection established");
    PKLOGI("board IP: %s", WiFi.localIP().toString().c_str());

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
    pk_i2c_begin(PK_SW_NONE);
#if CONF_LIB_I2C_RUN_SCANNER
    pk_i2c_scan();
#endif // CONF_LIB_I2C_RUN_SCANNER
#endif // CONF_LIB_I2C_ENABLED

    PKLOGI("setup finished");
}

void loop() {
    vTaskDelay(1000);
}
