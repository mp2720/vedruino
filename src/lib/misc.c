/*
 * TODO: сделать вывод информации о состоянии памяти через функции esp-idf.
 */

#include "misc.h"

#include "log.h"
#include <esp_ota_ops.h>
#include <stdlib.h>

#define MEM_LOGE(...) VDR_LOGE("mem", __VA_ARGS__)

static void inf_loop() {
    while (1)
        ;
}

void *misc_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        MEM_LOGE("malloc(size=%zu) failed", size);
        inf_loop();
    }
    return ptr;
}

void *misc_calloc(size_t n, size_t esize) {
    void *ptr = calloc(n, esize);
    if (!ptr) {
        MEM_LOGE("calloc(%zu, %zu) failed", n, esize);
        inf_loop();
    }
    return ptr;
}

void misc_free(void *ptr) {
    free(ptr);
}

void misc_running_partition(char out_label[MISC_PART_LABEL_SIZE]) {
    const esp_partition_t *part = esp_ota_get_running_partition();
    const char *lab = part->label;
    int i = 0;
    while (lab[i] && i < MISC_PART_LABEL_SIZE) {
        out_label[i] = lab[i];
        ++i;
    }
}
