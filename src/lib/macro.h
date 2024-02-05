#pragma once

#define UNUSED __attribute__((unused))

#ifdef __cplusplus
#define EXTERNC_BEGIN extern "C" {
#define EXTERNC_END }
#define EXTERNC extern "C"
#else
#define EXTERNC_BEGIN
#define EXTERNC_END
#define EXTERNC
#endif
