#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include <xtensa/config/system.h>
#include <xtensa/hal.h>


// pk_btrace_get_start(uint32_t *pc, uint32_t *sp, uint32_t *next_pc)

    .align      4
    .global     pk_btrace_get_start
    .type       pk_btrace_get_start, @function
pk_btrace_get_start:
    entry   a1, 32
    call8   xthal_window_spill
    l32e    a5, sp, -16
    s32i    a5, a2, 0
    l32e    a6, sp, -12
    l32e    a5, a6, -12
    s32i    a5, a3, 0
    l32e    a5, a6, -16
    s32i    a5, a4, 0
    retw
