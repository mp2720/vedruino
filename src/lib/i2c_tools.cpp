#include "i2c_tools.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "inc.h"
#include <Wire.h>

static SemaphoreHandle_t pk_i2c_mutex = NULL;
static QueueHandle_t current_i2c_line = NULL; // очередь длинной 1, хранит текущую линию i2c

#define I2C_MAX_WAIT_MS pdMS_TO_TICKS(1000)

static const char *TAG = "i2c_tools";
static pk_i2c_switcher_t i2c_switcher = PK_SW_NONE;

void pk_i2c_begin(pk_i2c_switcher_t switcher) {
    i2c_switcher = switcher;

    pk_i2c_mutex = xSemaphoreCreateRecursiveMutex();
    current_i2c_line = xQueueCreate(1, sizeof(int));
    if (!current_i2c_line) {
        PKLOGE("xQueueCreate failed");
    }
    int val = -1;
    if (xQueueSend(current_i2c_line, &val, I2C_MAX_WAIT_MS) != pdTRUE) {
        PKLOGE("xQueueSend failed");
    }

    Wire.begin();

    if (i2c_switcher != PK_SW_NONE) {
        pk_i2c_switch(0);
    }
}

void pk_i2c_lock() {
    BaseType_t res = xSemaphoreTakeRecursive(pk_i2c_mutex, I2C_MAX_WAIT_MS);
    if (res != pdTRUE) {
        PKLOGE("failed to take i2c_mutex");
    }
}

void pk_i2c_unlock() {
    if (xSemaphoreGiveRecursive(pk_i2c_mutex) != pdTRUE) {
        PKLOGE("failed to give i2c_mutex");
    }
}

static int pk_i2c_get_line() {
    int line;
    if (xQueuePeek(current_i2c_line, &line, I2C_MAX_WAIT_MS) != pdTRUE) {
        PKLOGE("failed to get i2c line from queqe");
        return -1;
    }
    return line;
}

static void pk_i2c_set_line(int line) {
    int current_line;
    pk_i2c_lock();
    if (xQueueReceive(current_i2c_line, &current_line, I2C_MAX_WAIT_MS) != pdTRUE) {
        pk_i2c_unlock();
        PKLOGE("failed to get i2c line from queqe");
        return;
    }
    if (xQueueSend(current_i2c_line, &line, I2C_MAX_WAIT_MS)) {
        pk_i2c_unlock();
        PKLOGE("failed to send i2c line to queqe");
        return;
    }
    pk_i2c_unlock();
}

void pk_i2c_switch(uint8_t i2c_line) {
    const int I2C_HUB_ADDR = 0x70;
    const int EN_MASK = 0x08;
    const int MAX_CHANNEL = 0x08;

    if (i2c_switcher == PK_SW_NONE) {
        PKLOGW("call pk_i2c_switch() without multiplexer");
        return;
    }
    if (i2c_line >= MAX_CHANNEL) {
        PKLOGE("i2c line out of range: %u", i2c_line);
        return;
    }

    if (pk_i2c_get_line() == i2c_line) {
        return;
    }

    pk_i2c_lock();

    Wire.beginTransmission(I2C_HUB_ADDR);

    if (i2c_switcher == PK_SW_PCA9547) {
        Wire.write(i2c_line | EN_MASK);
    } else if (i2c_switcher == PK_SW_PW548A) {
        Wire.write(0x01 << i2c_line);
    } else {
        PKLOGE("Incorrect i2c_switcher");
    }

    uint8_t res = Wire.endTransmission();
    if (res != 0) {
        PKLOGE("Switching transission fail: %u", res);
    } else {
        pk_i2c_set_line(i2c_line);
    }

    pk_i2c_unlock();
    return;
}

static void pk_i2c_scan_current_line() {
    pk_i2c_lock();

    PKLOGI("Start i2c scanning...");
    int devs_num = 0;
    for (int address = 8; address < 127; address++) {
        Wire.beginTransmission(address);
        int res = Wire.endTransmission();
        switch (res) {
        case 0:
            PKLOGI("Found I2C device on 0x%.2x", address);
            devs_num++;
            break;
        case 1:
            PKLOGW("Data too long to fit in transmit buffer on 0x%.2x", address);
            break;
        case 2:
            // ненаход
            break;
        case 3:
            PKLOGW("Received NACK on transmit of data on 0x%.2x", address);
            break;
        case 4:
            PKLOGW("Other error on 0x%.2x", address);
            break;
        case 5:
            PKLOGW("Timeout on 0x%.2x", address);
            break;
        default:
            PKLOGW("Unknown result code Wire.endTransmission() -> %d", res);
        }
    }
    PKLOGI("Found %d i2c devices", devs_num);
    pk_i2c_unlock();
}

void pk_i2c_scan() {
    pk_i2c_lock();
    if (i2c_switcher == PK_SW_NONE) {
        pk_i2c_scan_current_line();
    } else {
        int last_line = pk_i2c_get_line();
        for (int line = 0x03; line <= 0x07; line++) {
            pk_i2c_switch(line);
            PKLOGI("Switch to %d line", line);
            pk_i2c_scan_current_line();
        }
        pk_i2c_switch(last_line);
    }
    pk_i2c_unlock();
}