#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "lib/mqtt.h"
#include "lib/log.h"
#include "lib/tcp.h"

void setup() {
    Serial.begin(115200);
    puts("start \"void setup();\"");

    delay(1000);

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

void loop() {
    delay(1000);
}