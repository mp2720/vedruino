#include <Arduino.h>
#include <WiFi.h>

#include "fl_esp_mqtt.h"
#include "private.h"

void echo_qos0(const char * topic, const char * data, size_t size) {
    fl_mqtt_publish("echo/0", data, size, 0, 0);
}

void echo_qos1(const char * topic, const char * data, size_t size) {
    fl_mqtt_publish("echo/1", data, size, 1, 0);
}

void echo_qos2(const char * topic, const char * data, size_t size) {
    fl_mqtt_publish("echo/2", data, size, 2, 0);
}

#define QOS 0
fl_topic_t topics_set[] = { //{"name", callback_t, qos}
    {"test/0", echo_qos0, 0},
    {"test/1", echo_qos1, 1},
    {"test/2", echo_qos2, 2},
};

void setup() {
    Serial.begin(115200);

    delay(1000);

    WiFi.mode(WIFI_STA); //Optional
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    puts("\nConnecting WIFI");
    while(WiFi.status() != WL_CONNECTED){
        printf(".");
        delay(100);
    }
    puts("\nConnected to the WiFi network");
    printf("Local ESP32 IP: %s\n", WiFi.localIP().toString().c_str());


    fl_mqtt_init();
    fl_mqtt_connect(HOST, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD);
    puts("\nConnecting MQTT");
    while (fl_mqtt_is_connected() != 1){
        printf(".");
        delay(100);
    }
    puts("\nConnected to the MQTT broker");

    fl_mqtt_subscribe_topics(topics_set, sizeof(topics_set)/sizeof(topics_set[0]));

}

void loop() {
    delay(1000);
}