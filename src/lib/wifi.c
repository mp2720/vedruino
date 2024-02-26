#include "inc.h"

/* #if CONF_LIB_WIFI_ENABLED */
#if 0

#include <esp_err.h>
#include <esp_event.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <stdbool.h>
#include <string.h>

static const char *TAG = "wifi";

#define WIFI_CONNECTED_BIT 0x1
#define WIFI_FAIL_BIT 0x2

static EventGroupHandle_t s_wifi_event_group;

static void event_handler(
    PK_UNUSED void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
) {
    static int s_retry_num = 0;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_err_t res = esp_wifi_connect();
        if (res != ESP_OK) {
            PKLOGE("esp_wifi_connect() error: %d - %s", (int)res, esp_err_to_name(res));
        }

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < CONF_LIB_WIFI_RETRY) {
            esp_err_t res = esp_wifi_connect();
            if (res != ESP_OK) {
                PKLOGE("esp_wifi_connect() error: %d - %s", (int)res, esp_err_to_name(res));
            }
            s_retry_num++;
            PKLOGW("Retry to connect to the AP %d", s_retry_num);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        PKLOGI("Board ip: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

bool pk_wifi_connect() {
    if (strlen(CONF_LIB_WIFI_SSID) + 1 > 32) {
        PKLOGE("SSID to long: %u/32 bytes", strlen(CONF_LIB_WIFI_SSID));
        return 0;
    }
    if (strlen(CONF_LIB_WIFI_PASSWORD) + 1 > 64) {
        PKLOGE("Password to long: %u/64 bytes", strlen(CONF_LIB_WIFI_SSID));
        return 0;
    }
    /*
    esp_err_t nvres = nvs_flash_init();
    if (res != ESP_OK) {
        PKLOGE("nvs_flash_init() error: %d - %s", (int)nvres, esp_err_to_name(nvres));
        return 0;
    }
    */
    s_wifi_event_group = xEventGroupCreate();

    esp_err_t res = esp_netif_init();
    if (res != ESP_OK) {
        PKLOGE("esp_netif_init() error: %d - %s", (int)res, esp_err_to_name(res));
        return 0;
    }

    res = esp_event_loop_create_default();
    if (res != ESP_OK) {
        PKLOGE("esp_event_loop_create_default() error: %d - %s", (int)res, esp_err_to_name(res));
        return 0;
    }

    if (!esp_netif_create_default_wifi_sta()) {
        PKLOGE("esp_netif_create_default_wifi_sta() return NULL");
        return 0;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = 0;

    res = esp_wifi_init(&cfg);
    if (res != ESP_OK) {
        PKLOGE("esp_wifi_init() error: %d - %s", (int)res, esp_err_to_name(res));
        return 0;
    }

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    res = esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &event_handler,
        NULL,
        &instance_any_id
    );
    if (res != ESP_OK) {
        PKLOGE(
            "esp_event_handler_instance_register(WIFI_EVENT) error: %d - %s",
            (int)res,
            esp_err_to_name(res)
        );
        return 0;
    }

    res = esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &event_handler,
        NULL,
        &instance_got_ip
    );
    if (res != ESP_OK) {
        PKLOGE(
            "esp_event_handler_instance_register(IP_EVENT) error: %d - %s",
            (int)res,
            esp_err_to_name(res)
        );
        return 0;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONF_LIB_WIFI_SSID,
            .password = CONF_LIB_WIFI_PASSWORD,
        }};

    res = esp_wifi_set_mode(WIFI_MODE_STA);
    if (res != ESP_OK) {
        PKLOGE("esp_wifi_set_mode() error: %d - %s", (int)res, esp_err_to_name(res));
        return 0;
    }

    res = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (res != ESP_OK) {
        PKLOGE("esp_wifi_set_config() error: %d - %s", (int)res, esp_err_to_name(res));
        return 0;
    }

    res = esp_wifi_start();
    if (res != ESP_OK) {
        PKLOGE("esp_wifi_start() error: %d - %s", (int)res, esp_err_to_name(res));
        return 0;
    }

    PKLOGI("Connecting to ssid: %s", CONF_LIB_WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if (bits & WIFI_CONNECTED_BIT) {
        PKLOGI("Connected to AP");
    } else if (bits & WIFI_FAIL_BIT) {
        PKLOGW("Failed to connect to AP");
    } else {
        PKLOGE("UNEXPECTED EVENT");
    }

    res = esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
    if (res != ESP_OK) {
        PKLOGE(
            "esp_event_handler_instance_unregister(IP_EVENT) error: %d - %s",
            (int)res,
            esp_err_to_name(res)
        );
        return 0;
    }

    res = esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
    if (res != ESP_OK) {
        PKLOGE(
            "esp_event_handler_instance_unregister(WIFI_EVENT) error: %d - %s",
            (int)res,
            esp_err_to_name(res)
        );
        return 0;
    }

    vEventGroupDelete(s_wifi_event_group);

    res = esp_wifi_set_ps(WIFI_PS_NONE);
    if (res != ESP_OK) {
        PKLOGE("esp_wifi_set_ps() error: %d - %s", (int)res, esp_err_to_name(res));
        return 0;
    }

    return 1;
}

#endif // CONF_LIB_WIFI_ENABLED
