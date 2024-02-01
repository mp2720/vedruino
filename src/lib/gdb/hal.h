/*
 * Hardware abstractions.
 *
 * This version is tested on ESP32-WROOM, but other versions with Xtensa CPU may work fine too.
 * RISC-V CPUs are not supported.
 *
 */

#define GDB_STACK_SIZE 4096

#ifndef __ASSEMBLY__
#include <stdint.h>

// Put char to UART.
// FIXME: current implementations of IO functions are using ESP ROM component, which is not
// recommended.
void gdb_putc(char c);
// Get char from UART.
// Blocks until any character is available.
// FIXME: gdb can hang when invalid command is sent from client, add timeout to fix it.
char gdb_getc();

// When any exception occurs, the assembly code from hal.S vector handlers is executed.
// Vector handlers should be as small as possible (64 bytes is maximum) and placed into IRAM0.
// Because of that, control flows to assembly gdb_xxx_exc0 for storing registers and setting up
// stack. After, these functions (gdb_xxx_exc1) are called. Do not call them from any other place.

void gdb_debug_exc1();
void gdb_kernel_exc1();
void gdb_user_exc1();
void gdb_double_exc1();

// These functions are called on startup and exception handling.

/* void gdb_disable_ints(); */
void gdb_disable_wdts();

// These functions are called before starting/resuming the program.

/* extern inline void gdb_enable_ints(); */
extern inline void gdb_enable_wdts();

#endif // __ASSEMBLY__
