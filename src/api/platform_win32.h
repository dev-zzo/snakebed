#ifndef __SNAKEBED_PLATFORM_WIN32_H
#define __SNAKEBED_PLATFORM_WIN32_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <intrin.h>

#ifdef PLATFORM_CURRENT
#undef PLATFORM_CURRENT
#endif
#define PLATFORM_CURRENT PLATFORM_WINNT

#ifndef NULL
#define NULL (void *)0
#endif

/* https://msdn.microsoft.com/en-us/library/s3f49ktz.aspx */
typedef unsigned char Sb_byte_t;

typedef size_t Sb_size_t;
typedef long Sb_ssize_t;

typedef long long Sb_long64_t;
typedef unsigned long long Sb_ulong64_t;

#define Sb_Mul32x32As64(a, b) \
    ((Sb_long64_t)__emul((a), (b)))

#define Sb_Mulu32x32As64(a, b) \
    ((Sb_ulong64_t)__emulu((a), (b)))

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_PLATFORM_WIN32_H
