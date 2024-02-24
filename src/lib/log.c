#include "inc.h"

#include <freertos/semphr.h>
#include <soc/cpu.h>

FILE *pk_log_uartout;

SemaphoreHandle_t pk_btrace_mutex;
static StaticSemaphore_t btrace_mutex_st;

void pk_log_init() {
    pk_log_uartout = stdout;

    pk_btrace_mutex = xSemaphoreCreateMutexStatic(&btrace_mutex_st);
    PK_ASSERT(pk_btrace_mutex);
}

void pk_btrace_get_start(uint32_t *pc, uint32_t *sp, uint32_t *next_pc);

void pk_log_btrace(FILE *out) {
    uint32_t pc, sp, next_pc;
    pk_btrace_get_start(&pc, &sp, &next_pc);
    fputs("PKBTRACE START\n", out);
    while (sp != 0) {
        fprintf(out, "PKBTRACE 0x%08x:0x%08x\n", esp_cpu_process_stack_pc(pc), sp);
        void *frame_base = (void *)sp;
        pc = next_pc;
        next_pc = *(uint32_t *)(frame_base - 16);
        sp = *(uint32_t *)(frame_base - 12);
    }
    fputs("PKBTRACE END\n", out);
}
