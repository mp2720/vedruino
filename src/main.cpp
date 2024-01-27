#include "conf.h"
#include "lib/log.h"
#include "lib/misc.h"
#include "lib/ota.h"
#include <Arduino.h>
#include <WiFi.h>

#define MAIN_LOGI(...) VDR_LOGI("main", __VA_ARGS__)

void setup() {
    Serial.begin(CONF_BAUD);

    MAIN_LOGI("executing startup delay");
    delay(CONF_STARTUP_DELAY);

    char running_part[MISC_PART_LABEL_SIZE];
    misc_running_partition(running_part);
    MAIN_LOGI("running %s", running_part);
    MAIN_LOGI("built on " __DATE__ " at " __TIME__);

    WiFi.begin(CONF_WIFI_SSID, CONF_WIFI_PASSWD);
    MAIN_LOGI("connecting to %s...", CONF_WIFI_SSID);
    MAIN_LOGI("board MAC: %s", WiFi.macAddress().c_str());
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }
    MAIN_LOGI("WiFi connection established");
    MAIN_LOGI("board IP: %s", WiFi.localIP().toString().c_str());

#if CONF_TCP_OTA_ENABLED
    tcp_ota_start_server(CONF_TCP_OTA_PORT);
#endif

    MAIN_LOGI("setup() finished");
}

void loop() {
    delay(10000);
}
