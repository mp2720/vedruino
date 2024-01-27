#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Функции выводят сообщения об ошибки и запускают бесконечный цикл если произошла ошибка.
 */
void *misc_malloc(size_t size);
void *misc_calloc(size_t n, size_t esize);
void misc_free(void *ptr);

/* null-terminated, from esp_ota_ops.h */
#define MISC_PART_LABEL_SIZE 17
void misc_running_partition(char out_label[MISC_PART_LABEL_SIZE]);

#ifdef __cplusplus
}
#endif
