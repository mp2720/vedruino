#pragma once

#include <assert.h>

#define PK_ASSERT(x) assert(x)

/* #define PK_ASSERT(x) \ */
/*     do { \ */
/*         if (!(x)) { \ */
/*             pk_assert_failed(TAG, #x, __FILE__, __LINE__); \ */
/*         } \ */
/*     } while (0) */

/* void pk_assert_failed(const char *expr_str, const char *file, int line); */
