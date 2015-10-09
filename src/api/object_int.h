#ifndef __SNAKEBED_OBJECT_INT_H
#define __SNAKEBED_OBJECT_INT_H
#ifdef __cplusplus
extern "C" {
#endif

/* NOTE: this should actually be a fixed-size 32-bit signed integer. */
typedef Sb_ssize_t SbInt_Native_t;

/* Type for digits not carrying the sign bit. */
typedef unsigned short SbInt_Digit_t;
/* Type for digits carrying the sign bit. */
typedef signed short SbInt_SignDigit_t;
/* Type to contain double-digit result. */
typedef unsigned long SbInt_DoubleDigit_t;

#define SbInt_DIGIT_BITS 16

typedef struct {
    Sb_size_t length : 31;
    Sb_size_t native : 1;
    union {
        SbInt_Native_t value;
        SbInt_Digit_t *digits;
    } u;
} SbInt_Value;

/* Define the int object structure. */
typedef struct _SbIntObject {
    SbObject_HEAD;
    SbInt_Value v;
} SbIntObject;

extern SbTypeObject *SbInt_Type;

/* Verify the given object is of type int.
   Returns: 1 if true, 0 otherwise. */
#define SbInt_CheckExact(p) \
    (Sb_TYPE(p) == SbInt_Type)

#define SbInt_Check(p) \
    (SbType_IsSubtype(Sb_TYPE(p), SbInt_Type))

/* Construct an int object from a C long.
   Returns: New reference. */
SbObject *
SbInt_FromNative(SbInt_Native_t ival);

/* Return the object's value as C SbInt_Native_t.
   Returns: value; if out of bounds -- -1 and sets overflow_flag */
SbInt_Native_t
SbInt_AsNativeOverflow(SbObject *op, int *overflow_flag);

/* Return the object's value as C SbInt_Native_t.
   Returns: value; if out of bounds -- -1 and raises an exception */
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
