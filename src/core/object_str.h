#ifndef __SNAKEBED_OBJECT_STR_H
#define __SNAKEBED_OBJECT_STR_H
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"

extern SbTypeObject *SbStr_Type;

/* Verify the given object is of type str.
   Returns: 1 if true, 0 otherwise. */
int
SbStr_CheckExact(SbObject *p);


SbObject *
SbStr_FromString(const char *v);

SbObject *
SbStr_FromStringAndSize(const char *v, Sb_ssize_t len);

Sb_ssize_t
SbStr_GetSizeUnsafe(SbObject *p);

char *
SbStr_AsStringUnsafe(SbObject *p);

Sb_ssize_t
SbStr_GetSize(SbObject *p);

char *
SbStr_AsString(SbObject *p);

long
_SbStr_Hash(SbObject *p);

long
_SbStr_HashString(const unsigned char *v, Sb_ssize_t len);

int
_SbStr_Eq(SbObject *p1, SbObject *p2);

int
_SbStr_EqString(SbObject *p1, const char *p2);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_STR_H
