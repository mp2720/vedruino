#include "conf.h"

// Долбоебы преопределили IPADDR_NONE, нужно включить раньше
#include <WiFi.h>

#include "lib/lib.h"

static const char *TAG = "main";

void setup() {
    pk_log_init();

    PKLOGI("executing startup delay");
    delay(CONF_BOARD_STARTUP_DELAY);

    const char *rp = pk_running_part_label();
    PKLOGI("running %s parition", rp == NULL ? "UNKNOWN" : rp);
    PKLOGI("built on " __DATE__ " at " __TIME__);

#if CONF_WIFI_ENABLED
    WiFi.begin(CONF_WIFI_SSID, CONF_WIFI_PASSWORD);
    PKLOGI("connecting to %s...", CONF_WIFI_SSID);
    PKLOGI("board MAC: %s", WiFi.macAddress().c_str());
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }
    PKLOGI("WiFi connection established");
    PKLOGI("board IP: %s", WiFi.localIP().toString().c_str());
#endif // CONF_WIFI_ENABLED

#if CONF_SYSMON_ENABLED
    sysmon_start();
#endif

#if CONF_MDNS_ENABLED
    if (!pk_mdns_init())
        PKLOGE("failed to init mdns");
#endif // CONF_MDNS_ENABLED

#if CONF_NETLOG_ENABLED
    if (!pk_netlog_init())
        PKLOGE("failed to init netlog");
#endif // CONF_NETLOG_ENABLED

#if CONF_OTA_ENABLED
    if (!ota_server_start())
        PKLOGE("failed to start ota server");
#endif

    PKLOGI("setup() finished");
}

void loop() {
    delay(10000);
}
