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

void init_sensors();

// звук
typedef struct {
    int v[3];
} noise_t;

noise_t get_noise(); // более 100 есть шум

// огонь
typedef struct {
    float v[3];
} fire_t;

void fire_calibrate();
void init_fire();
fire_t get_fire();

// землетрясение

void init_axel();
float get_axel();

// газы
typedef struct {
    float v[3];
} air_t;

void init_air();
air_t get_air();

//протечка
void init_water();
float get_water();