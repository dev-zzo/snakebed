#include "runtime.h"

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

const void *
Sb_MemChr(const void *p, int value, Sb_size_t count)
{
    const char *s = (const char *)p;
    while (count--) {
        if (*s == value) {
            return s;
        }
        ++s;
    }
    return NULL;
}

const void *
Sb_MemRChr(const void *p, int value, Sb_size_t count)
{
    const char *s = (const char *)p + count;
    while (count--) {
        --s;
        if (*s == value) {
            return s;
        }
    }
    return NULL;
}

void
Sb_MemSet(void *ptr, int ch, Sb_size_t count)
{
    char *p = (char *)ptr;
    while (count--) {
        *p++ = (char)ch;
    }
}

/* The following routines are adapted from http://effbot.org/zone/stringlib.htm */

typedef unsigned long Sb_BloomMask_t;

#define STRINGLIB_BLOOM_KEY(ch) \
    (1UL << ((ch) & (sizeof(Sb_BloomMask_t) * 8 - 1)))
#define STRINGLIB_BLOOM_ADD(mask, ch) \
    ((mask) |= STRINGLIB_BLOOM_KEY(ch))
#define STRINGLIB_BLOOM_TEST(mask, ch) \
    ((mask) &  STRINGLIB_BLOOM_KEY(ch))

Sb_ssize_t
Sb_MemMem(const char *str, Sb_ssize_t str_len, const char *pat, Sb_ssize_t pat_len)
{
    Sb_BloomMask_t mask;
    Sb_ssize_t skip;
    Sb_ssize_t i, mlast, w;
    const char *ss, *pp;

    if (str_len < pat_len) {
        return -1;
    }

    /* Special case: 1-char pattern */
    if (pat_len == 1) {
        const char *p;

        p = (const char *)Sb_MemChr(str, pat[0], str_len);
        if (p) {
            return p - str;
        }
        return -1;
    }

    /* Special case: same length strings */
    if (pat_len == str_len) {
        return Sb_MemCmp(str, pat, pat_len) ? -1 : 0;
    }

    /* create compressed boyer-moore delta 1 table */
    mlast = pat_len - 1;
    skip = mlast - 1;
    mask = 0;

    /* process pattern */
    for (i = 0; i < mlast; i++) {
        STRINGLIB_BLOOM_ADD(mask, pat[i]);
        if (pat[i] == pat[mlast]) {
            skip = mlast - i - 1;
        }
    }
    STRINGLIB_BLOOM_ADD(mask, pat[mlast]);

    ss = str + mlast;
    pp = pat + mlast;
    w = str_len - pat_len;
    for (i = 0; i <= w; i++) {
        if (ss[i] == pp[0]) {
            Sb_ssize_t j;

            /* candidate match */
            for (j = 0; j < mlast; j++) {
                if (str[i+j] != pat[j]) {
                    goto miss;
                }
            }
            /* got a match! */
            return i;

miss:
            /* miss: check if next character is part of pattern */
            if (!STRINGLIB_BLOOM_TEST(mask, ss[i+1])) {
                i = i + pat_len;
            }
            else {
                i = i + skip;
            }
        } else {
            /* skip: check if next character is part of pattern */
            if (!STRINGLIB_BLOOM_TEST(mask, ss[i+1])) {
                i = i + pat_len;
            }
        }
    }
    return -1;
}

Sb_ssize_t
Sb_MemRMem(const char *str, Sb_ssize_t str_len, const char *pat, Sb_ssize_t pat_len)
{
    Sb_BloomMask_t mask;
    Sb_ssize_t skip;
    Sb_ssize_t i, mlast, w;

    if (str_len < pat_len) {
        return -1;
    }

    /* Special case: 1-char pattern */
    if (pat_len == 1) {
        const char *p;

        p = (const char *)Sb_MemRChr(str, pat[0], str_len);
        if (p) {
            return p - str;
        }
        return -1;
    }

    /* Special case: same length strings */
    if (pat_len == str_len) {
        return Sb_MemCmp(str, pat, pat_len) ? -1 : 0;
    }

    /* create compressed boyer-moore delta 1 table */
    mlast = pat_len - 1;
    skip = mlast - 1;
    mask = 0;

    /* process pattern */
    STRINGLIB_BLOOM_ADD(mask, pat[0]);
    for (i = mlast; i > 0; i--) {
        STRINGLIB_BLOOM_ADD(mask, pat[i]);
        if (pat[i] == pat[0]) {
            skip = i - 1;
        }
    }

    w = str_len - pat_len;
    for (i = w; i >= 0; i--) {
        if (str[i] == pat[0]) {
            Sb_ssize_t j;

            /* candidate match */
            for (j = mlast; j > 0; j--) {
                if (str[i+j] != pat[j]) {
                    goto miss;
                }
            }
            /* got a match! */
            return i;

miss:
            /* miss: check if next character is part of pattern */
            if (i > 0 && !STRINGLIB_BLOOM_TEST(mask, str[i-1])) {
                i = i - pat_len;
            }
            else {
                i = i - skip;
            }
        } else {
            /* skip: check if next character is part of pattern */
            if (i > 0 && !STRINGLIB_BLOOM_TEST(mask, str[i-1])) {
                i = i - pat_len;
            }
        }
    }
    return -1;
}

