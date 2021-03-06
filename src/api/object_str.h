#ifndef __SNAKEBED_OBJECT_STR_H
#define __SNAKEBED_OBJECT_STR_H
#ifdef __cplusplus
extern "C" {
#endif

/* Define the str object structure. */
typedef struct _SbStrObject {
    SbObject_HEAD_VAR;
    long stored_hash;
    Sb_byte_t items[1];
} SbStrObject;

extern SbTypeObject *SbStr_Type;

/* Verify the given object is of type str.
   Returns: 1 if true, 0 otherwise. */
#define SbStr_CheckExact(p) \
    (Sb_TYPE(p) == SbStr_Type)

/* Construct a str object from a C string.
   Returns: New reference. */
SbObject *
SbStr_FromString(const char *v);

/* Construct a str object from a C array.
   If `v` is NULL, nothing is copied.
   Returns: New reference. */
SbObject *
SbStr_FromStringAndSize(const void *v, Sb_ssize_t len);

/* Construct a str object from a printf-like format string.
   Returns: New reference. */
SbObject *
SbStr_FromFormat(const char *format, ...);
SbObject *
SbStr_FromFormatVa(const char *format, va_list va);

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

/* Truncate a string; new length must be less than current.
   NOTE: ONLY to be used on freshly created strings.
   Returns: 0 if OK, -1 otherwise. */
int
SbStr_Truncate(SbObject *p, Sb_ssize_t new_length);

/* Check whether or not p2 starts with p2.
   Returns: 1 if yes, 0 if no, -1 on error.
*/
int
SbStr_StartsWithString(SbObject *p1, const char *p2);

/* Join strings in `iterable` using `glue`.
   Returns: New reference. */
SbObject *
SbStr_Join(SbObject *glue, SbObject *iterable);

/* Create a new string, which is the same as the source but jsutified.
   Returns: New reference. */
SbObject *
SbStr_JustifyLeft(SbObject *p, Sb_ssize_t width, char filler);
SbObject *
SbStr_JustifyCenter(SbObject *p, Sb_ssize_t width, char filler);
SbObject *
SbStr_JustifyRight(SbObject *p, Sb_ssize_t width, char filler);


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
