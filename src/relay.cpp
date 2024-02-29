#include "app.h"

#include <PCA9536.h>

static PCA9536 pca9536;

static const char *TAG = "relay";

void app_relay_init() {
    pk_i2c_switch(3);
    pca9536.reset();
    pca9536.setMode(IO_OUTPUT);
}

void app_relay_set(int sel, bool state) {
    if (sel == 0) {
        pca9536.setState(IO0, state ? IO_HIGH : IO_LOW);
    } else {
        pca9536.setState(IO1, state ? IO_HIGH : IO_LOW);
    }
}
