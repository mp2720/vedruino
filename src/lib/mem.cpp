#include "mem.h"

#include "log.h"
#include <cstdlib>

#define MEM_LOGE(...) VDR_LOGE("mem", __VA_ARGS__)

static void inf_loop() {
    while (1)
        ;
}

void *mem::malloc(size_t size) {
    void *ptr = std::malloc(size);
    if (!ptr) {
        MEM_LOGE("malloc(size=%zu) failed", size);
        inf_loop();
    }
    return ptr;
}

void *mem::calloc(size_t n, size_t esize) {
    void *ptr = std::calloc(n, esize);
    if (!ptr) {
        MEM_LOGE("calloc(n=%zu, esize=%zu) failed", n, esize);
        inf_loop();
    }
    return ptr;
}

void mem::free(void *ptr) {
    std::free(ptr);
}
