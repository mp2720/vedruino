#pragma once

/*
gp 17-16 line 7
gp 13-4 line 6
gp 15-14 line 5
gp 23-5 line 4
*/

// датчик звука mcp3221 0x4e - 7
// датчик звука mcp3221 0x4e - 6
// датчик звука mcp3221 0x4e - 5
// дачик пламени MGS_FR403 0x39 - 7
// дачик пламени MGS_FR403 0x39 - 6
// дачик пламени MGS_FR403 0x39 - 5
// датчик газа 0x58 - 5
// датчик газа 0x58 - 6
// датчик газа 0x58 - 7
// акселерометр MPU6050 0x69 - all
// датчик уровня воды 0x4d - all

typedef struct {
    int noise[3];
    float fire[3];
    float gas[3];
    float axel;
    float amperage;
    float water_flow;
    float water_overflow;
} appSensorData_t;

extern appSensorData_t app_sensors;

void app_sensors_init();
void app_sensors_poll();
void app_sensors_calibrate_fire();
