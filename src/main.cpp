#include "lib/lib.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

static const char *TAG = "main";

void setup() {
    Serial.begin(CONF_BAUD);

    DFLT_LOGI(TAG, "executing startup delay");
    delay(CONF_STARTUP_DELAY);

    char running_part[MISC_PART_LABEL_SIZE];
    misc_running_partition(running_part);
    DFLT_LOGI(TAG, "running %s", running_part);
    DFLT_LOGI(TAG, "built on " __DATE__ " at " __TIME__);

    WiFi.begin(CONF_WIFI_SSID, CONF_WIFI_PASSWD);
    /* esp_wifi_set_ps(WIFI_PS_NONE); */
    DFLT_LOGI(TAG, "connecting to %s...", CONF_WIFI_SSID);
    DFLT_LOGI(TAG, "board MAC: %s", WiFi.macAddress().c_str());
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }
    DFLT_LOGI(TAG, "WiFi connection established");
    DFLT_LOGI(TAG, "board IP: %s", WiFi.localIP().toString().c_str());

#if CONF_SYSMON_ENABLED
    sysmon_start();
#endif

#if CONF_TCP_OTA_ENABLED
    ota_server_start();
#endif

    DFLT_LOGI(TAG, "setup() finished");
}

void loop() {
    delay(10000);
}
