#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include "config.h"
#include "lib/mqtt.h"
#include "lib/log.h"
#include "lib/tcp.h"

static const char * TAG = "MAIN";

void start_wifi() {
    WiFi.mode(WIFI_STA); //Optional
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    puts("\nConnecting WIFI");
    while(WiFi.status() != WL_CONNECTED){
        printf("."); fflush(stdout);
        delay(100);
    }
    puts("\nConnected to the WiFi network");
    printf("Local ESP32 IP: %s\n", WiFi.localIP().toString().c_str());
}

void start_log() {
    log_socket = tcp_connect(LOG_HOST, LOG_PORT);
    log_output = tcp_log_printf;
}

void start_mqtt() {
    mqtt_init();
    mqtt_connect(MQTT_HOST, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD);
    while (!mqtt_is_connected()) {
        printf("."); fflush(stdout);
        delay(100);
    }
    puts("");
    //mqtt_subscribe_topics(topics, sizeof(topics)/sizeof(topics[0]));
}

void setup() {
    start_wifi();
    start_log();
    start_mqtt();
}

void loop() {
    delay(1000);
}