#include "inc.h"

FILE *pk_log_uartout;

void pk_log_init() {
    pk_log_uartout = stdout;
}
