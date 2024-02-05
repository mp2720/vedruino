#include "esp_heap_caps.h"
#include "lib.h"
#include <esp_freertos_hooks.h>
#include <esp_ipc_isr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdbool.h>

#define LOG_TASK_STACK_SIZE 2048
#define LOG_TASK_PRIORITY 24

const char *TAG = "sysmon";

static void log_task(void *);
static bool idle_hook();
static inline void log_and_reset_cpu_load(int idx);

static size_t total_heap;
static TaskHandle_t log_task_hnd;

#if CONF_SYSMON_DUALCORE
static int idle_ticks[2];
static SemaphoreHandle_t idle_mutex[2];
#else
static int idle_ticks[1];
static SemaphoreHandle_t idle_mutex[1];
#endif // CONF_SYSMON_DUALCORE

// Для тестирования
/* #pragma GCC push_options */
/* #pragma GCC optimize("O0") */
/* void loop_task(UNUSED void *p) { */
/*     while (1) { */
/*         vTaskDelay(2); */
/*         for (int i = 0; i < 1e6; ++i) { */
/*         } */
/*     } */
/* } */
/* #pragma GCC pop_options */

int sysmon_start() {
    total_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);

#if CONF_SYSMON_MONITOR_CPU || CONF_SYSMON_MONITOR_HEAP || CONF_SYSMON_MONITOR_TASKS
#if CONF_SYSMON_MONITOR_CPU
    idle_ticks[0] = 0;

#if CONF_SYSMON_DUALCORE
    idle_ticks[1] = 0;
#endif // CONF_SYSMON_DUALCORE

    if ((idle_mutex[0] = xSemaphoreCreateMutex()) == NULL
#if CONF_SYSMON_DUALCORE
        || (idle_mutex[1] = xSemaphoreCreateMutex()) == NULL
#endif // CONF_SYSMON_DUALCORE
    ) {
        SM_LOGE("xSemaphoreCreateMutex() failed");
        return -1;
    }

    esp_err_t err;
    if ((err = esp_register_freertos_idle_hook_for_cpu(&idle_hook, 0)) != ESP_OK) {
        SM_LOGE("esp_register_freertos_idle_hook_for_cpu() failed: %s", esp_err_to_name(err));
        return -1;
    }

#if CONF_SYSMON_DUALCORE
    if ((err = esp_register_freertos_idle_hook_for_cpu(&idle_hook, 1)) != ESP_OK) {
        SM_LOGE("esp_register_freertos_idle_hook_for_cpu() failed: %s", esp_err_to_name(err));
        return -1;
    }
#endif // CONF_SYSMON_DUALCORE
#endif // CONF_SYSMON_MONITOR_CPU

    if (xTaskCreate(&log_task, "sysmon_log", LOG_TASK_STACK_SIZE, NULL, LOG_TASK_PRIORITY,
                    &log_task_hnd) != pdPASS) {
        SM_LOGE("xTaskCreate() failed");
        return -1;
    }

    // Для тестирования
    /* xTaskCreate(&loop_task, "loop0", LOG_TASK_STACK_SIZE, NULL, 1, NULL); */
    /* xTaskCreate(&loop_task, "loop1", LOG_TASK_STACK_SIZE, NULL, 1, NULL); */

    SM_LOGD("started");
#endif // CONF_SYSMON_MONITOR_CPU || CONF_SYSMON_MONITOR_HEAP || CONF_SYSMON_MONITOR_TASKS

    return 0;
}

void sysmon_dump_heap_stat() {
    size_t used_heap = total_heap - heap_caps_get_free_size(MALLOC_CAP_DEFAULT);

    float total_f, used_f;
    char total_s[MISC_BISUFFIX_SIZE], used_s[MISC_BISUFFIX_SIZE];
    misc_hum_size(total_heap, &total_f, total_s);
    misc_hum_size(used_heap, &used_f, used_s);

    float mem_usage = 100.f * used_f / total_f;
    ESP_LOGI(TAG, "heap %.2f%sB/%.2f%sB (%.1f%%)", used_f, used_s, total_f, total_s, mem_usage);
}

void sysmon_dump_tasks() {
    unsigned tasks_num = uxTaskGetNumberOfTasks();
    TaskSnapshot_t *snapshots = malloc(tasks_num * sizeof(*snapshots));
    if (snapshots == NULL) {
        ESP_LOGE(TAG, "malloc() failed");
        return;
    }
    UNUSED unsigned tcb_size;
    tasks_num = uxTaskGetSnapshotAll(snapshots, tasks_num, &tcb_size);
    if (tasks_num == 0)
        return;

    for (size_t i = 0; i < tasks_num; ++i) {
        TaskHandle_t thnd = snapshots[i].pxTCB;

        const char *name = pcTaskGetName(thnd);
        void *stack_end = snapshots[i].pxEndOfStack;
        void *stack_top = snapshots[i].pxTopOfStack;
        const char *state;
        switch (eTaskGetState(thnd)) {
        case eReady:
            state = "READY";
            break;
        case eRunning:
            state = "RUN";
            break;
        case eBlocked:
            state = "BLOCK";
            break;
        case eSuspended:
            state = "SUSP";
            break;
        case eDeleted:
            state = "DEL";
            break;
        default:
            state = "INV";
            break;
        }
        unsigned cur_pr = uxTaskPriorityGet(thnd);

        ESP_LOGI(TAG, "TASK %s %p %s %u %p %p", name, thnd, state, cur_pr, stack_end, stack_top);
    }

    free(snapshots);
}

void sysmon_pause() {
#if CONF_SYSMON_MONITOR_CPU || CONF_SYSMON_MONITOR_HEAP || CONF_SYSMON_MONITOR_TASKS
    vTaskSuspend(log_task_hnd);
    SM_LOGD("paused");
#endif // CONF_SYSMON_MONITOR_CPU || CONF_SYSMON_MONITOR_HEAP || CONF_SYSMON_MONITOR_TASKS
}

void sysmon_resume() {
#if CONF_SYSMON_MONITOR_CPU || CONF_SYSMON_MONITOR_HEAP || CONF_SYSMON_MONITOR_TASKS
    vTaskResume(log_task_hnd);
    SM_LOGD("resumed");
#endif // CONF_SYSMON_MONITOR_CPU || CONF_SYSMON_MONITOR_HEAP || CONF_SYSMON_MONITOR_TASKS
}

static void log_task(UNUSED void *p) {
    while (1) {
        vTaskDelay(CONF_SYSMON_LOG_INTERVAL_MS / portTICK_PERIOD_MS);

#if CONF_SYSMON_MONITOR_HEAP
        sysmon_dump_heap_stat();
#endif // CONF_SYSMON_MONITOR_HEAP

#if CONF_SYSMON_MONITOR_TASKS
        sysmon_dump_tasks();
#endif // CONF_SYSMON_MONITOR_TASKS

#if CONF_SYSMON_MONITOR_CPU
        log_and_reset_cpu_load(0);
#if CONF_SYSMON_DUALCORE
        log_and_reset_cpu_load(1);
#endif // CONF_SYSMON_DUALCORE
#endif // CONF_SYSMON_MONITOR_CPU
    }
}

static bool idle_hook() {
    int core_id = xPortGetCoreID();
    // blockTime=0 -> не блокирует.
    if (xSemaphoreTake(idle_mutex[core_id], 0)) {
        ++idle_ticks[core_id];
        xSemaphoreGive(idle_mutex[core_id]);
    }

    // Хук должен вызываться каждый тик.
    // (см. esp_register_freertos_idle_hook_for_cpu())
    return true;
}

static inline void log_and_reset_cpu_load(int core_id) {
    // blockTime=0 -> не блокирует.
    if (xSemaphoreTake(idle_mutex[core_id], 0)) {
        TickType_t ticks = CONF_SYSMON_LOG_INTERVAL_MS / portTICK_PERIOD_MS;
        float load = 1.f - MIN(idle_ticks[core_id] / (float)ticks, 1.f);
        load *= 100.f;
        ESP_LOGI(TAG, "cpu%d %.1f%%", core_id, load);
        idle_ticks[core_id] = 0;
        xSemaphoreGive(idle_mutex[core_id]);
    }
}
