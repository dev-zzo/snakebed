
#include "platform.h"
#include <malloc.h>

void *
Sb_Malloc(Sb_size_t size)
{
    return malloc(size);
}

void *
Sb_Realloc(void *ptr, Sb_size_t new_size)
{
    return realloc(ptr, new_size);
}

void
Sb_Free(void *ptr)
{
    free(ptr);
}

void
Sb_StrCpy(char *dst, const char *src)
{
    while (*dst++ = *src++);
}

int
Sb_StrCmp(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

void
Sb_MemCpy(void *dst, const void *src, Sb_size_t count)
{
    memcpy(dst, src, count);
}
