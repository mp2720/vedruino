#include "log.h"

FILE *pk_log_uartout;

bool pk_log_init() {
    pk_log_uartout = stdout;
    return true;
}
