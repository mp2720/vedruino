#include "sensor.h"
#include "MCP3221.h"
#include "lib.h"
#include "lib/i2c_tools.h"

// датчик звука mcp3221 0x4e - 7
// датчик звука mcp3221 0x4e - 6
// датчик звука mcp3221 0x4e - 5


static const char *TAG = "sensors";


//датчик звука
MCP3221 noise1(0x4e);
MCP3221 noise2(0x4e);
MCP3221 noise3(0x4e);
noise_t get_noise() {
    float res[3];

    pk_i2c_lock();
    pk_i2c_switch(7);
    res[0] = noise1.getVoltage();
    pk_i2c_switch(6);
    res[1] = noise1.getVoltage();
    pk_i2c_switch(5);
    res[2] = noise1.getVoltage();
    pk_i2c_unlock();

    if (res[0] == 0) {
        PKLOGE("Noise sensor on 0x4e - 7 err");
    }
    if (res[1] == 0) {
        PKLOGE("Noise sensor on 0x4e - 6 err");
    }
    if (res[2] == 0) {
        PKLOGE("Noise sensor on 0x4e - 5 err");
    }

    noise_t ret;
    ret.v[0] = (int)fabs(1250.f-res[0]);
    ret.v[1] = (int)fabs(1250.f-res[1]);
    ret.v[2] = (int)fabs(1250.f-res[2]);

    return ret;
}