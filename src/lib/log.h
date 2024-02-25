#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdio.h>

extern FILE *pk_log_uartout;

extern SemaphoreHandle_t pk_btrace_mutex;

void pk_log_init(void);

#define PK_BTRACE_MUTEX_TAKE xSemaphoreTake(pk_btrace_mutex, portMAX_DELAY)
#define PK_BTRACE_MUTEX_GIVE xSemaphoreGive(pk_btrace_mutex)

// Вывод бектрейса потокобезопасен, но если одна задача, выводящая бектрейс, будет прервана другой,
// которая тоже начнет вывод бектрейса, то он перемешается и будет бесполезен.
// Чтобы такой проблемы не было, можно использовать PK_BTRACE_MUTEX_TAKE и PK_BTRACE_MUTEX_GIVE.
// Вызывать только если со стеком всё в порядке.
void pk_log_btrace(FILE *out);
