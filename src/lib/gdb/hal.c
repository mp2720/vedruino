#include "hal.h"

#include "xtensa_regs.h"
#include <esp_rom_uart.h>
#include <hal/wdt_hal.h>

// We need to save both CPUs registers, because when program is paused, user can switch between
// threads (FreeRTOS tasks) that are executed on different CPUS.
volatile uint32_t gdb_regfile[GDB_REGS * 2];

#define GDB_REGFILE_CPU

// uint32_t is used to be sure that alignment is ok.
// Used only when exception is handled.
// When exception is handled, gdb is running only on one CPU, so one stack is enough.
volatile uint32_t gdb_stack[GDB_STACK_SIZE / sizeof(uint32_t)];

// 1 if gdb is running on any CPU, 0 if not.
// Change only in exception handler and only with atomic CPU instructions (s32c1i for Xtensa).
volatile uint32_t gdb_spinlock = 0;

void gdb_putc(char c) {
    esp_rom_uart_putc(c);
}

char gdb_getc() {
    uint8_t c;
    while (esp_rom_uart_rx_one_char(&c) != 0)
        ;
    return c;
}

#ifndef RWDT_HAL_CONTEXT_DEFAULT
#define RWDT_HAL_CONTEXT_DEFAULT()                                                                 \
    { .inst = WDT_RWDT, .rwdt_dev = RWDT_DEV_GET() }
#endif

#ifndef RWDT_DEV_GET
#define RWDT_DEV_GET() &RTCCNTL
#endif

static wdt_hal_context_t rtc_wdt_ctx = RWDT_HAL_CONTEXT_DEFAULT();
static bool rtc_wdt_ctx_enabled = false;
static wdt_hal_context_t wdt0_context = {.inst = WDT_MWDT0, .mwdt_dev = &TIMERG0};
static bool wdt0_context_enabled = false;
#if SOC_TIMER_GROUPS >= 2
static wdt_hal_context_t wdt1_context = {.inst = WDT_MWDT1, .mwdt_dev = &TIMERG1};
static bool wdt1_context_enabled = false;
#endif // SOC_TIMER_GROUPS

void gdb_disable_wdts(void) {
    wdt0_context_enabled = wdt_hal_is_enabled(&wdt0_context);
#if SOC_TIMER_GROUPS >= 2
    wdt1_context_enabled = wdt_hal_is_enabled(&wdt1_context);
#endif
    rtc_wdt_ctx_enabled = wdt_hal_is_enabled(&rtc_wdt_ctx);

    /*Task WDT is the Main Watchdog Timer of Timer Group 0 */
    if (true == wdt0_context_enabled) {
        wdt_hal_write_protect_disable(&wdt0_context);
        wdt_hal_disable(&wdt0_context);
        wdt_hal_feed(&wdt0_context);
        wdt_hal_write_protect_enable(&wdt0_context);
    }

#if SOC_TIMER_GROUPS >= 2
    /* Interupt WDT is the Main Watchdog Timer of Timer Group 1 */
    if (true == wdt1_context_enabled) {
        wdt_hal_write_protect_disable(&wdt1_context);
        wdt_hal_disable(&wdt1_context);
        wdt_hal_feed(&wdt1_context);
        wdt_hal_write_protect_enable(&wdt1_context);
    }
#endif // SOC_TIMER_GROUPS >= 2

    if (true == rtc_wdt_ctx_enabled) {
        wdt_hal_write_protect_disable(&rtc_wdt_ctx);
        wdt_hal_disable(&rtc_wdt_ctx);
        wdt_hal_feed(&rtc_wdt_ctx);
        wdt_hal_write_protect_enable(&rtc_wdt_ctx);
    }
}
