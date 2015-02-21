#ifndef __SNAKEBED_PLATFORM_WIN32_H
#define __SNAKEBED_PLATFORM_WIN32_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#ifndef NULL
#define NULL (void *)0
#endif

typedef size_t Sb_size_t;
typedef long Sb_ssize_t;
typedef unsigned char Sb_byte_t;

#define Sb_BZero(ptr, size) \
    memset(ptr, 0, size)


#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_PLATFORM_WIN32_H
