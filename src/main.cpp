#include "conf.h"
#include "lib/lib.h"
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

static const char *TAG = "MAIN";

void start_wifi() {
    WiFi.mode(WIFI_STA); // Optional
    WiFi.begin(CONF_WIFI_SSID, CONF_WIFI_PASSWD);
    puts("\nConnecting WIFI");
    while (WiFi.status() != WL_CONNECTED) {
        printf(".");
        fflush(stdout);
        delay(100);
    }
    puts("\nConnected to the WiFi network");
    printf("Local ESP32 IP: %s\n", WiFi.localIP().toString().c_str());
}

void start_log() {
    log_socket = tcplog_connect(CONF_LOG_HOST, CONF_LOG_PORT);
    log_output = tcplog_printf;
}

void start_mqtt() {
    mqtt_init();
    mqtt_connect(CONF_MQTT_HOST, CONF_MQTT_PORT, CONF_MQTT_USER, CONF_MQTT_PASSWD);
    while (!mqtt_is_connected()) {
        printf(".");
        fflush(stdout);
        delay(100);
    }
    puts("");
    // mqtt_subscribe_topics(topics, sizeof(topics)/sizeof(topics[0]));
}

void setup() {
    Serial.begin(CONF_BAUD);
    delay(CONF_STARTUP_DELAY);

    start_wifi();
    start_log();
    start_mqtt();

#if CONF_TCP_OTA_ENABLED
    ota_server_start(CONF_TCP_OTA_PORT);
#endif
}

void loop() {
    delay(1000);
}
