#ifndef __SNAKEBED_OBJECT_INT_H
#define __SNAKEBED_OBJECT_INT_H
#ifdef __cplusplus
extern "C" {
#endif

/* NOTE: this should actually be a fixed-size 32-bit signed integer. */
typedef Sb_ssize_t SbInt_Native_t;

/* Define the int object structure. */
typedef struct _SbIntObject {
    SbObject_HEAD;
    SbInt_Native_t value;
} SbIntObject;

extern SbTypeObject *SbInt_Type;

/* Verify the given object is of type int.
   Returns: 1 if true, 0 otherwise. */
#define SbInt_CheckExact(p) \
    (Sb_TYPE(p) == SbInt_Type)

#define SbInt_Check(p) \
    (SbType_IsSubtype(Sb_TYPE(p), SbInt_Type))

/* Returns the maximum value of the int object.
   Returns: Plain C data. */
SbInt_Native_t
SbInt_GetMax(void);

/* Returns the minimum value of the int object.
   Returns: Plain C data. */
SbInt_Native_t
SbInt_GetMin(void);

/* Construct an int object from a C long.
   Returns: New reference. */
SbObject *
SbInt_FromNative(SbInt_Native_t ival);

#define SbInt_AsNativeUnsafe(p) \
    (((SbIntObject *)p)->value)

/* Return the object's value as C SbInt_Native_t.
   WARNING: No conversions are performed.
   Returns: Plain C data. */
SbInt_Native_t
SbInt_AsNative(SbObject *op);

/* Construct an int object from a C string.
   Returns: New reference. */
SbObject *
SbInt_FromString(const char *str, const char **pend, unsigned base);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_INT_H
