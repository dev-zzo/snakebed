#include "bstrap.h"

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
