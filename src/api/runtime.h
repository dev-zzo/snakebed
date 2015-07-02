/*
 * SnakeBed Bootstrap Routines
 */
#ifndef __SNAKEBED_BSTRAP_H
#define __SNAKEBED_BSTRAP_H

#include "platform.h"

#define Sb_OffsetOf(type, member) ((Sb_size_t)(&((type *)0)->member))

Sb_size_t
Sb_StrLen(const char *s);

void
Sb_StrCpy(char *dst, const char *src);

int
Sb_StrCmp(const char *s1, const char *s2);

void
Sb_MemCpy(void *dst, const void *src, Sb_size_t count);

int
Sb_MemCmp(const void *p1, const void *p2, Sb_size_t count);

const void *
Sb_MemChr(const void *p, int value, Sb_size_t count);
const void *
Sb_MemRChr(const void *p, int value, Sb_size_t count);

void
Sb_BZero(void *ptr, Sb_size_t size);

void
Sb_MemSet(void *ptr, int ch, Sb_size_t count);

char *
Sb_ULtoA(unsigned long x, unsigned radix);
char *
Sb_LtoA(long x, int radix);
int
Sb_AtoL(const char *str, const char **pend, unsigned radix, long *result);
int
Sb_AtoUL(const char *str, const char **pend, unsigned radix, unsigned long *result);

int Sb_IsDigit(char c);
int Sb_IsWhiteSpace(char c);

Sb_ssize_t
Sb_MemMem(const char *str, Sb_ssize_t str_len, const char *pat, Sb_ssize_t pat_len);
Sb_ssize_t
Sb_MemRMem(const char *str, Sb_ssize_t str_len, const char *pat, Sb_ssize_t pat_len);

#endif // __SNAKEBED_BSTRAP_H