#ifndef __SNAKEBED_OBJECT_INT_H
#define __SNAKEBED_OBJECT_INT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"

extern SbTypeObject *SbInt_Type;

/* Verify the given object is of type int.
   Returns: 1 if true, 0 otherwise. */
int
SbInt_CheckExact(SbObject *op);

/* Returns the maximum value of the int object.
   Returns: Plain C data. */
long
SbInt_GetMax();

/* Construct an int object from a C long.
   Returns: New reference. */
SbObject *
SbInt_FromLong(long ival);

/* Return the object's value as C long.
   WARNING: No conversions are performed.
   Returns: Plain C data. */
long
SbInt_AsLong(SbObject *op);

/* Construct an int object from a C string.
   Returns: New reference. */
SbObject *
SbInt_FromString(const char *str, const char **pend, unsigned base);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_INT_H
