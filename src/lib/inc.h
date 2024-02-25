#pragma once

#include "../conf.h"
#include "macro.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// === i2c ===

#if CONF_LIB_I2C_ENABLED
#include "i2c_tools.h"
#endif // CONF_LIB_I2C_ENABLED

PK_EXTERNC_BEGIN

// === log ===

#include "log.h"

#ifdef PK_LIB_INCLUDE_FROM_APP
#define PKLOG_LEVEL CONF_LOG_APP_LEVEL
#else
#define PKLOG_LEVEL CONF_LOG_LIB_LEVEL
#endif // PK_LIB_INCLUDE_FROM_APP

#include "log_defs.h"

// === wifi ===

#if CONF_LIB_WIFI_ENABLED
bool pk_wifi_connect();
#endif // CONF_LIB_WIFI_ENABLED

// === ip ===

#if CONF_LIB_IP_ENABLED
#include "ip.h"
#endif // CONF_LIB_IP_ENABLED

// === mdns ===

#if CONF_LIB_MDNS_ENABLED
bool pk_mdns_init();
#endif // CONF_LIB_MDNS_ENABLED

// === netlog ===

#if CONF_LIB_NETLOG_ENABLED
void pk_netlog_init(void);
#endif // CONF_LIB_NETLOG_ENABLED

// === ota ===

#if CONF_LIB_OTA_ENABLED
bool ota_server_start();
#endif // CONF_LIB_OTA_ENABLED

// === sysmon ===

#if 0
int sysmon_start();
void sysmon_dump_heap_stat();
void sysmon_dump_tasks();
void sysmon_pause();
void sysmon_resume();
#endif

// === mqtt ====

#if CONF_LIB_MQTT_ENABLED
#include "mqtt.h"
#endif // CONF_LIB_MQTT_ENABLED

// === json ===

#if CONF_LIB_JSON_ENABLED
#include "lwjson/lwjson.h"
#include "lwjson/pk_lwjson.h"
#endif // CONF_LIB_JSON_ENABLED

// === pk_assert ===
#include "pk_assert.h"

// === misc ===

const char *pk_running_part_label();
const char *pk_reset_reason_str();

// Размер суффикса к количеству информации в байтах (null-terminated).
#define PK_BISUFFIX_SIZE 3
// Перевести размер информации в вид <множитель><суффикс>.
// Используются суффиксы для степени 2 (Ki, Mi, ..., Yi).
void pk_hum_size(float size, float *out_f, char out_suf[PK_BISUFFIX_SIZE]);

PK_EXTERNC_END
