#include "conf.h"
#include "lib/log.h"
#include "lib/ota.h"
#include <Arduino.h>
#include <WiFi.h>

#define MAIN_LOGI(...) VDR_LOGI("main", __VA_ARGS__)

void setup() {
    delay(3000);

    Serial.begin(CONF_BAUD);

    WiFi.begin(CONF_WIFI_SSID, CONF_WIFI_PASSWD);
    MAIN_LOGI("connecting to %s...", CONF_WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }
    MAIN_LOGI("WiFi connection established");
    MAIN_LOGI("board IP: %s", WiFi.localIP().toString().c_str());

#if CONF_OTA_ENABLED
    ota::dump_info();
    ota::update();
#endif
}

void loop() {
    delay(10000);
}
