#include "app.h"
#include "lib/i2c_tools.h"
#include "sensor.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "main";

static void setup_lib(void) {
    pk_log_init();

    const char *rp = pk_running_part_label();
    PKLOGI("running %s parition", rp == NULL ? "UNKNOWN" : rp);
    PKLOGI("built on " __DATE__ " at " __TIME__);

#if CONF_LIB_WIFI_ENABLED
    PK_ASSERT(pk_wifi_connect());
#endif // CONF_LIB_WIFI_ENABLED

#if CONF_LIB_MDNS_ENABLED
    if (!pk_mdns_init())
        PKLOGE("failed to start mdns");
#endif // CONF_LIB_MDNS_ENABLED

#if CONF_LIB_NETLOG_ENABLED
    pk_netlog_init();
#endif // CONF_LIB_NETLOG_ENABLED

#if CONF_LIB_OTA_ENABLED
    if (!ota_server_start())
        PKLOGE("failed to start ota server");
#endif // CONF_LIB_OTA_ENABLEDB

#if CONF_LIB_MQTT_ENABLED
    if (!pk_mqtt_connect())
        PKLOGE("failed to connect mqtt");
#endif // CONF_LIB_MQTT_ENABLED

#if CONF_LIB_I2C_ENABLED
    pk_i2c_begin(PK_SW_PCA9547);
#if CONF_LIB_I2C_RUN_SCANNER
    pk_i2c_scan();
#endif // CONF_LIB_I2C_RUN_SCANNER
#endif // CONF_LIB_I2C_ENABLED
}

static void setup_app() {
    // Put app setup code here
    // ...
}

void setup() {
    TickType_t start_lib_tick = xTaskGetTickCount();
    setup_lib();
    TickType_t end_lib_tick = xTaskGetTickCount();
    PKLOGI("setup_lib() finished in %.1f secs", (end_lib_tick - start_lib_tick) / 1000.f);

    TickType_t ticks_to_wait = PK_MAX(
        (TickType_t)0,
        (TickType_t)((long)start_lib_tick + (long)pdMS_TO_TICKS(CONF_MAIN_SETUP_WAIT_UNTIL_MS) -
                     (long)end_lib_tick)
    );
    PKLOGI("waiting %.1f secs before setup_app() call", pdTICKS_TO_MS(ticks_to_wait) / 1000.f);
    xTaskDelayUntil(&start_lib_tick, pdMS_TO_TICKS(CONF_MAIN_SETUP_WAIT_UNTIL_MS));

    TickType_t start_app_tick = xTaskGetTickCount();
    setup_app();
    TickType_t end_app_tick = xTaskGetTickCount();
    PKLOGI(
        "setup_app() finished in %.1f secs",
        pdTICKS_TO_MS(end_app_tick - start_app_tick) / 1000.f
    );
}

void loop() {
    noise_t d = get_noise();

    PKLOGI("Noise: %d %d %d", d.v[0], d.v[1], d.v[2]);
    //vTaskDelay(100);
}