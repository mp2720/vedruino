#include "lib.h"
#include <esp_err.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_timer.h>

static const char *TAG = "flash_test";

char buf[4096];

// 500 KiB/s
void test_flash_write() {
    const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);
    size_t blks = part->size / 4096;

    esp_err_t err;
    if ((err = esp_partition_erase_range(part, 0, part->size)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_partition_erase_range %s", esp_err_to_name(err));
        return;
    }

    int64_t t1 = esp_timer_get_time();
    for (size_t i = 0; i < blks; ++i) {
        if ((err = esp_partition_write(part, i * 4096, buf, 4096)) != ESP_OK) {
            ESP_LOGE(TAG, "esp_partition_write %s", esp_err_to_name(err));
            return;
        }
    }

    float dt_secs = (esp_timer_get_time() - t1) / 1e6f;

    ESP_LOGI(TAG, "erase&write %zu bytes in %f secs (%f b/s)", part->size, dt_secs,
             part->size / dt_secs);
}

// 9.98 MiB/s
void test_flash_read() {
    int64_t t1 = esp_timer_get_time();
    const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);
    for (size_t i = 0; i < part->size; i += 4096) {
        esp_err_t err = esp_partition_read(part, i, buf, 4096);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_partition_read %s", esp_err_to_name(err));
            return;
        }
    }

    float dt_secs = (esp_timer_get_time() - t1) / 1e6f;

    ESP_LOGI(TAG, "read %zu bytes in %f secs (%f b/s)", part->size, dt_secs, part->size / dt_secs);
}
