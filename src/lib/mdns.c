#include "inc.h"

#include <esp_err.h>
#include <mdns.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <stdbool.h>

#if CONF_MDNS_ENABLED

static const char *TAG = "mdns";

bool pk_mdns_init(void) {
    PKLOGD("trying to get hostname from nvs flash");

    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        PKLOGE("nvs_flash_init() %s", esp_err_to_name(err));
        return false;
    }

    nvs_handle_t nvs_hd;
    err = nvs_open("mdns", NVS_READONLY, &nvs_hd);
    if (err != ESP_OK) {
        PKLOGE("nvs_open() %s", esp_err_to_name(err));
        return false;
    }

    size_t hostname_size;
    nvs_get_str(nvs_hd, "hostname", NULL, &hostname_size);
    char hostname[hostname_size];
    nvs_get_str(nvs_hd, "hostname", hostname, &hostname_size);

    PKLOGI("hostname %s.local", hostname);

    err = mdns_init();
    if (err != ESP_OK) {
        PKLOGE("mdns_init() %s", esp_err_to_name(err));
        return false;
    }

    err = mdns_hostname_set(CONF_MDNS_HOSTNAME);
    if (err != ESP_OK) {
        PKLOGE("mdns_hostname_set() %s", esp_err_to_name(err));
        return false;
    }

    PKLOGI("mdns started");
    return true;
}

#endif // CONF_MDNS_ENABLED
