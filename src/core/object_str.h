#ifndef __SNAKEBED_OBJECT_STR_H
#define __SNAKEBED_OBJECT_STR_H
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"

/* Define the str object structure. */
typedef struct _SbStrObject {
    SbObject_HEAD_VAR;
    long stored_hash;
    Sb_byte_t items[1];
} SbStrObject;

extern SbTypeObject *SbStr_Type;

/* Verify the given object is of type str.
   Returns: 1 if true, 0 otherwise. */
int
SbStr_CheckExact(SbObject *p);

/* Construct a str object from a C string.
   Returns: New reference. */
SbObject *
SbStr_FromString(const char *v);

/* Construct a str object from a C array.
   Returns: New reference. */
SbObject *
SbStr_FromStringAndSize(const void *v, Sb_ssize_t len);

/* Obtain the str's length.
   WARNING: no type checks are performed. */
#define SbStr_GetSizeUnsafe(p) \
    Sb_COUNT(p)

/* Obtain the pointer to the str's buffer.
   WARNING: no type checks are performed. */
#define SbStr_AsStringUnsafe(p) \
    (((SbStrObject *)p)->items)

/* Obtain the str's length. */
Sb_ssize_t
SbStr_GetSize(SbObject *p);

/* Obtain the pointer to the str's buffer. */
const Sb_byte_t *
SbStr_AsString(SbObject *p);

/* METHODS USED INTERNALLY */

long
_SbStr_Hash(SbObject *p);

long
_SbStr_HashString(const Sb_byte_t *v, Sb_ssize_t len);

int
_SbStr_Eq(SbObject *p1, SbObject *p2);

int
_SbStr_EqString(SbObject *p1, const char *p2);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_STR_H
