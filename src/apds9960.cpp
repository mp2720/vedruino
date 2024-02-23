#include "apds9960.h"
#include "Adafruit_APDS9960.h"
#include "lib.h"
#include "lib/i2c_tools.h"

Adafruit_APDS9960 apds9960;

static const char *TAG = "apds9960";

void apds9960_init() {
    pk_i2c_lock();
    if (!apds9960.begin()) {
        PKLOGE("Failed to initialize apds9960");
    }
    apds9960.enableColor(true);
    apds9960.enableProximity(true);
    pk_i2c_unlock();
}

apds9960_data_t apds9960_get() {
    pk_i2c_lock();
    apds9960_data_t result;

    while (!apds9960.colorDataReady()) {
        delay(10);
    }
    apds9960.getColorData(&result.red, &result.green, &result.blue, &result.clear);
    pk_i2c_unlock();
    return result;
}
