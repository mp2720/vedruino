#pragma once

#include <cstddef>

namespace mem {
    void *malloc(size_t size);
    void *calloc(size_t n, size_t esize);
    void free(void *ptr);
}
