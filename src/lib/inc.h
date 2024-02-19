#pragma once

#include "../conf.h"
#include "macro.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

EXTERNC_BEGIN

// === log ===

extern FILE *pk_log_uartout;
void pk_log_init();

#ifdef PK_LIB_INCLUDE_FROM_APP
#define PKLOG_LEVEL CONF_LOG_APP_LEVEL
#else
#define PKLOG_LEVEL CONF_LOG_LIB_LEVEL
#endif // PK_LIB_INCLUDE_FROM_APP
       
#include "log_defs.h"

// === wifi ===

#if CONF_WIFI_ENABLED
bool pk_wifi_connect();
#endif //CONF_WIFI_ENABLED

// === ip ===

#include "ip.h"

// === mdns ===

#if CONF_MDNS_ENABLED
bool pk_mdns_init();
#endif // CONF_MDNS_ENABLED

// === netlog ===

#if CONF_NETLOG_ENABLED
bool pk_netlog_init(void);
#endif // CONF_NETLOG_ENABLED

// === ota ===

#if CONF_OTA_ENABLED
bool ota_server_start();
#endif // CONF_OTA_ENABLED

// === sysmon ===

#if CONF_SYSMON_ENABLED
int sysmon_start();
void sysmon_dump_heap_stat();
void sysmon_dump_tasks();
void sysmon_pause();
void sysmon_resume();
#endif // CONF_SYSMON_ENABLED

// === mqtt ====

#include "mqtt.h"

// === misc ===

const char *pk_running_part_label();
const char *pk_reset_reason_str();

// Размер суффикса к количеству информации в байтах (null-terminated).
#define MISC_BISUFFIX_SIZE 3
// Перевести размер информации в вид <множитель><суффикс>.
// Используются суффиксы для степени 2 (Ki, Mi, ..., Yi).
void pk_hum_size(size_t size, float *out_f, char out_suf[MISC_BISUFFIX_SIZE]);

EXTERNC_END
