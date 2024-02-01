#include "gdb.h"

#include "hal.h"
#include <freertos/FreeRTOS.h>
#include <stdio.h>

void test_bpnt() {
    asm("break.n 1\n");
}

void gdb_task_cpu0(void *p) {
    gdb_disable_wdts();
    printf("task0 on CPU %d\n", xPortGetCoreID());
    test_bpnt();
}

void gdb_task_cpu1(void *p) {
    printf("task1 on CPU %d\n", xPortGetCoreID());
    while (1) {
    }
}
