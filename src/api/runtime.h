#ifndef __SNAKEBED_RUNTIME_H
#define __SNAKEBED_RUNTIME_H

/* SnakeBed runtime support, abstracts OS services and replaces (not so standard) C library */

#include "platform.h"

#define Sb_OffsetOf(type, member) ((Sb_size_t)(&((type *)0)->member))

/* NUL-terminated string manipulation routines */

Sb_size_t
SbRT_StrLen(const char *s);

void
SbRT_StrCpy(char *dst, const char *src);

int
SbRT_StrCmp(const char *s1, const char *s2);

/* Buffer manipulation routines */

void
SbRT_MemCpy(void *dst, const void *src, Sb_size_t count);

int
SbRT_MemCmp(const void *p1, const void *p2, Sb_size_t count);

void
SbRT_BZero(void *ptr, Sb_size_t size);

void
SbRT_MemSet(void *ptr, int ch, Sb_size_t count);

const void *
SbRT_MemChr(const void *p, int value, Sb_size_t count);
const void *
SbRT_MemRChr(const void *p, int value, Sb_size_t count);

Sb_ssize_t
SbRT_MemMem(const char *str, Sb_ssize_t str_len, const char *pat, Sb_ssize_t pat_len);
Sb_ssize_t
SbRT_MemRMem(const char *str, Sb_ssize_t str_len, const char *pat, Sb_ssize_t pat_len);

/* Numeric conversion routines */

char *
Sb_ULtoA(unsigned long x, unsigned radix);
char *
Sb_LtoA(long x, int radix);
int
Sb_AtoL(const char *str, const char **pend, unsigned radix, long *result);
int
Sb_AtoUL(const char *str, const char **pend, unsigned radix, unsigned long *result);

/* Character type check routines */

int Sb_IsDigit(char c);
int Sb_IsWhiteSpace(char c);

#endif // __SNAKEBED_RUNTIME_H
