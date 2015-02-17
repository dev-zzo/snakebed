#include "bstrap.h"

Sb_size_t
Sb_StrLen(const char *s)
{
    const char *p = s;
    while (*p++);
    return p - s;
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

int
Sb_MemCmp(const void *p1, const void *p2, Sb_size_t count)
{
    return memcmp(p1, p2, count);
}

