#include "conf.h"
#include "lib/lib.h"
#include <Arduino.h>
#include <WiFi.h>

static const char *TAG = "main";

extern "C" void udp_test();

void setup() {
    Serial.begin(CONF_BAUD);

    ESP_LOGI(TAG, "executing startup delay");
    delay(CONF_STARTUP_DELAY);

    char running_part[MISC_PART_LABEL_SIZE];
    misc_running_partition(running_part);
    ESP_LOGI(TAG, "running %s", running_part);
    ESP_LOGI(TAG, "built on " __DATE__ " at " __TIME__);

    WiFi.begin(CONF_WIFI_SSID, CONF_WIFI_PASSWD);
    ESP_LOGI(TAG, "connecting to %s...", CONF_WIFI_SSID);
    ESP_LOGI(TAG, "board MAC: %s", WiFi.macAddress().c_str());
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }
    ESP_LOGI(TAG, "WiFi connection established");
    ESP_LOGI(TAG, "board IP: %s", WiFi.localIP().toString().c_str());

#if CONF_TCP_OTA_ENABLED
    ota_server_start(CONF_TCP_OTA_PORT);
#endif

    udp_test();

    ESP_LOGI(TAG, "setup() finished");
}

void loop() {
    delay(10000);
}
