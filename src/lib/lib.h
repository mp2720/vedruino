#pragma once

#include "../conf.h"
#include "macro.h"
#include <stdbool.h>
#include <stddef.h>

#include "log.h"
#include "mqtt.h"
#include "tcplog.h"

#include <esp_err.h>

EXTERNC_BEGIN

// === ota ===

bool ota_server_start();

// === sysmon ===

int sysmon_start();
void sysmon_dump_heap_stat();
void sysmon_dump_tasks();
void sysmon_pause();
void sysmon_resume();

// === misc ===

/* null-terminated, from esp_ota_ops.h */
#define MISC_PART_LABEL_SIZE 17
void misc_running_partition(char out_label[MISC_PART_LABEL_SIZE]);

// Размер суффикса к количеству информации в байтах (null-terminated).
#define MISC_BISUFFIX_SIZE 3
// Перевести размер информации в вид <множитель><суффикс>.
// Используются суффиксы для степени 2 (Ki, Mi, ..., Yi).
void misc_hum_size(size_t size, float *out_f, char out_suf[MISC_BISUFFIX_SIZE]);

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

EXTERNC_END
