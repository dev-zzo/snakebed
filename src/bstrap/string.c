#include "bstrap.h"

void
Sb_BZero(void *ptr, Sb_size_t size)
{
    char *p = (char *)ptr;
    while (size--) {
        *p++ = 0;
    }
}

Sb_size_t
Sb_StrLen(const char *s)
{
    const char *p;

    for (p = s; *p; ++p);
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
    /* TODO: implement me. */
    return strcmp(s1, s2);
}

void
Sb_MemCpy(void *dst, const void *src, Sb_size_t count)
{
    char *d = (char *)dst;
    const char *s = (const char *)src;

    while (count--) {
        *d++ = *s++;
    }
}

int
Sb_MemCmp(const void *p1, const void *p2, Sb_size_t count)
{
    /* TODO: implement me. */
    return memcmp(p1, p2, count);
}

