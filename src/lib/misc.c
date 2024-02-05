#include "lib.h"

#include <esp_ota_ops.h>
#include <stdlib.h>

static const char *TAG = "MEM";

void misc_running_partition(char out_label[MISC_PART_LABEL_SIZE]) {
    const esp_partition_t *part = esp_ota_get_running_partition();
    const char *lab = part->label;
    int i = 0;
    while (lab[i] && i < MISC_PART_LABEL_SIZE) {
        out_label[i] = lab[i];
        ++i;
    }
}

void misc_hum_size(size_t size, float *out_f, char *out_suf) {
    const char sufs[] = "\0KMGTPEZY";

    *out_f = size;

    size_t suf_i = 0;
    while (*out_f > 1024.f && suf_i < sizeof(sufs) - 2) {
        *out_f /= 1024.f;
        ++suf_i;
    }

    if (suf_i == 0) {
        out_suf[0] = 0;
    } else {
        out_suf[0] = sufs[suf_i];
        out_suf[1] = 'i';
        out_suf[2] = 0;
    }
}
