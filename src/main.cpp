#include "conf.h"

#include "esp32-hal.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "lib.h"
#include "lib/i2c_tools.h"
#include "lib/macro.h"
#include "lib/mqtt.h"

static const char *TAG = "main";

void setup() {
    pk_log_init();

    PKLOGI("executing startup delay");
    delay(CONF_BOARD_STARTUP_DELAY);

    const char *rp = pk_running_part_label();
    PKLOGI("running %s parition", rp == NULL ? "UNKNOWN" : rp);
    PKLOGI("built on " __DATE__ " at " __TIME__);

#if CONF_WIFI_ENABLED
    if (!pk_wifi_connect())
        PKLOGE("failed to connect to wifi");
#endif // CONF_WIFI_ENABLED

#if CONF_SYSMON_ENABLED
    sysmon_start();
#endif

#if CONF_MDNS_ENABLED
    if (!pk_mdns_init())
        PKLOGE("failed to init mdns");
#endif // CONF_MDNS_ENABLED

#if CONF_NETLOG_ENABLED
    pk_netlog_init();
#endif // CONF_NETLOG_ENABLED

#if CONF_OTA_ENABLED
    if (!ota_server_start())
        PKLOGE("failed to start ota server");
#endif

#if CONF_MQTT_ENABLED
    if (!pk_mqtt_connect())
        PKLOGE("failed to connect mqtt");
#endif

    pk_i2c_begin(PK_SW_NONE);
    pk_i2c_scan();

    PKLOGI("setup() finished");
}

void loop() {
    vTaskDelete(NULL);
}
