#ifndef __SNAKEBED_PLATFORM_H
#define __SNAKEBED_PLATFORM_H
#ifdef __cplusplus
extern "C" {
#endif

#include "platform_win32.h"

void *
Sb_Malloc(Sb_size_t size);

void *
Sb_Realloc(void *ptr, Sb_size_t new_size);

void
Sb_Free(void *ptr);

void
Sb_StrCpy(char *dst, const char *src);

int
Sb_StrCmp(const char *s1, const char *s2);

void
Sb_MemCpy(void *dst, const void *src, Sb_size_t count);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_PLATFORM_H
