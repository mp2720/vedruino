#include "conf.h"

// Долбоебы преопределили IPADDR_NONE, нужно включить раньше
#include <WiFi.h>

#include "lib/lib.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static const char *TAG = "main";

static int cnt[10];
static bool done[10];

extern "C" void tcp_test();

static void task(UNUSED void *p) {
    int n = (int)p;
    for (int i = 0; i < 1000; ++i) {
        printf("%daaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
               "aaaaaaaaaaaaaaaaaaaaaa\n",
               n);
        /* fprintf(nlog_uartout, "%d\n", n); */
        cnt[n]++;
        vTaskDelay(1000);
    }
    /* done[n] = true; */
    /* vTaskDelete(NULL); */
}

void setup() {
    /* Serial.begin(CONF_LOG_UART_BAUD); */
    pk_log_init();

    PKLOGI("executing startup delay");
    delay(CONF_BOARD_STARTUP_DELAY);

    char running_part[MISC_PART_LABEL_SIZE];
    misc_running_partition(running_part);
    PKLOGI("running %s", running_part);
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

    /* tcp_test(); */

#if CONF_MDNS_ENABLED
    if (!pk_mdns_init())
        PKLOGE("failed to init mdns");
#endif // CONF_MDNS_ENABLED

#if CONF_NETLOG_ENABLED
    if (!pk_netlog_init()) {
        PKLOGE("failed to init netlog");
    }
#endif // CONF_NETLOG_ENABLED

    /* while (1) { */
    /*     PKLOGE("error %d", 1488); */
    /*     vTaskDelay(1); */
    /* } */

    /* int64_t t1 = esp_timer_get_time(); */

    xTaskCreate(&task, "task", 4096, 0, 1, NULL);
    /* xTaskCreate(&task, "task", 4096, (void *)1, 1, NULL); */
    /* xTaskCreate(&task, "task", 4096, (void *)2, 1, NULL); */
    /* xTaskCreate(&task, "task", 4096, (void *)3, 1, NULL); */
    /* xTaskCreate(&task, "task", 4096, (void *)4, 1, NULL); */
    /* xTaskCreate(&task, "task", 4096, (void *)5, 1, NULL); */
    /* xTaskCreate(&task, "task", 4096, (void *)6, 1, NULL); */
    /* xTaskCreate(&task, "task", 4096, (void *)7, 1, NULL); */
    /* xTaskCreate(&task, "task", 4096, (void *)8, 1, NULL); */
    /* xTaskCreate(&task, "task", 4096, (void *)9, 1, NULL); */

    /* while (1) { */
    /*     vTaskDelay(1000); */
    /*     bool alldone = true; */
    /*     for (int i = 0; i < 10; ++i) { */
    /*         alldone = alldone && done[i]; */
    /*     } */
    /*     if (alldone) */
    /*         break; */
    /* } */

    /* float secs = (esp_timer_get_time() - t1) / 1000000.f; */

    /* size_t total_cnt = 0; */
    /* for (int i = 0; i < 10; ++i) { */
    /*     total_cnt += cnt[i]; */
    /* } */

    /* total_cnt *= 103; */

    /* PKLOGI_UART("%zu bytes in %f secs (%f b/s)", total_cnt, secs, total_cnt / secs); */

    /* log_async_init(); */


#if CONF_TCP_OTA_ENABLED
    ota_server_start(CONF_TCP_OTA_PORT);
#endif

    /* while (1) { */
    /*     printf("aaaaaaaaaaaaaaaaaaaaaaaaaa\n"); */
    /*     vTaskDelay(100); */
    /* } */

    PKLOGI("setup() finished");
}

void loop() {
    delay(10000);
}
