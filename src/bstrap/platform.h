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

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_PLATFORM_H
