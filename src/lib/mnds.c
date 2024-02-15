#include "lib.h"

#include <esp_err.h>
#include <mdns.h>
#include <stdbool.h>

#if CONF_MDNS_ENABLED

static const char *TAG = "mdns";

bool pk_mdns_init(void) {
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        PKLOGE("mdns_init() %s", esp_err_to_name(err));
        return false;
    }

    err = mdns_hostname_set(CONF_MDNS_HOSTNAME);
    if (err != ESP_OK) {
        PKLOGE("mdns_hostname_set() %s", esp_err_to_name(err));
        return false;
    }

    return true;
}

#endif // CONF_MDNS_ENABLED
