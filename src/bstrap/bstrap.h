/*
 * SnakeBed Bootstrap Routines
 */
#ifndef __SNAKEBED_BSTRAP_H
#define __SNAKEBED_BSTRAP_H

#include "platform.h"

#define Sb_OffsetOf(st, m) ((Sb_size_t)(&((st *)0)->m))

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


#endif // __SNAKEBED_BSTRAP_H
