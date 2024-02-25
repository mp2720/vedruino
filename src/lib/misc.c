#include "inc.h"

#include <esp_ota_ops.h>
#include <esp_system.h>
#include <math.h>

const char *pk_running_part_label() {
    const esp_partition_t *part = esp_ota_get_running_partition();
    if (part == NULL)
        return NULL;
    return part->label;
}

void pk_hum_size(float size, float *out_f, char out_suf[PK_BISUFFIX_SIZE]) {
    const char sufs[] = "\0KMGTPEZY";

    size_t suf_i = 0;
    if (size != INFINITY && size != NAN) {
        while (size > 1024.f && suf_i < sizeof(sufs) - 2) {
            size /= 1024.f;
            ++suf_i;
        }
    }

    *out_f = size;

    out_suf[0] = sufs[suf_i];
    out_suf[1] = 'i';
    out_suf[2] = 0;
}

const char *pk_reset_reason_str() {
    switch (esp_reset_reason()) {
    case ESP_RST_POWERON:
        return "POWERON";
    case ESP_RST_EXT:
        return "EXIT";
    case ESP_RST_SW:
        return "SOFT";
    case ESP_RST_PANIC:
        return "PANIC";
    case ESP_RST_INT_WDT:
        return "INT_WDT";
    case ESP_RST_TASK_WDT:
        return "TASK_WDT";
    case ESP_RST_WDT:
        return "WDT";
    case ESP_RST_DEEPSLEEP:
        return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:
        return "BROWNOUT";
    case ESP_RST_SDIO:
        return "SDIO";
    /* case ESP_RST_USB: */
    /*     return "USB"; */
    /* case ESP_RST_JTAG: */
    /*     return "JTAG"; */
    case ESP_RST_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}
