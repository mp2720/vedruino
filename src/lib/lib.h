#pragma once

#include "../conf.h"
#include "macro.h"
#include <stdbool.h>
#include <stddef.h>

#include "ip.h"
#include "log.h"
#include "mqtt.h"

#include <esp_err.h>

EXTERNC_BEGIN

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

// === misc ===

/* null-terminated, from esp_ota_ops.h */
#define MISC_PART_LABEL_SIZE 17
void misc_running_partition(char out_label[MISC_PART_LABEL_SIZE]);

// Размер суффикса к количеству информации в байтах (null-terminated).
#define MISC_BISUFFIX_SIZE 3
// Перевести размер информации в вид <множитель><суффикс>.
// Используются суффиксы для степени 2 (Ki, Mi, ..., Yi).
void misc_hum_size(size_t size, float *out_f, char out_suf[MISC_BISUFFIX_SIZE]);

EXTERNC_END
