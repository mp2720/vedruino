#pragma once

#define UNUSED __attribute__((unused))
#define PACKED __attribute__((__packed__))

#ifdef __cplusplus
#define EXTERNC_BEGIN extern "C" {
#define EXTERNC_END }
#define EXTERNC extern "C"
#else
#define EXTERNC_BEGIN
#define EXTERNC_END
#define EXTERNC
#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define PK_STRINGIZE(x) _PK_STRINGIZE2(x)
#define _PK_STRINGIZE2(x) #x
