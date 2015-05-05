#ifndef __SNAKEBED_OBJECT_TUPLE_H
#define __SNAKEBED_OBJECT_TUPLE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"

/* Define the tuple object structure. */
typedef struct _SbTupleObject {
    SbObject_HEAD_VAR;
    SbObject *items[1];
} SbTupleObject;

extern SbTypeObject *SbTuple_Type;

/* Return true if p is a tuple object, 
   but not an instance of a subtype of the tuple type. */
#define SbTuple_CheckExact(p) \
    (Sb_TYPE(p) == SbTuple_Type)

/* Return a new tuple object with given size.
   Returns: New reference or NULL on failure. */
SbObject *
SbTuple_New(Sb_ssize_t length);

/* Construct a new tuple from the provided arguments.
   Note: This function does not "steal" references to passed objects.
   Returns: New reference or NULL on failure. */
SbObject *
SbTuple_Pack(Sb_ssize_t count, ...);
SbObject *
SbTuple_PackVa(Sb_ssize_t count, va_list va);

/* Unpack the tuple into the provided arguments.
   Arguments must be pointers to `SbObject *`, they will contain borrowed refs.
   Returns: 0 if OK, -1 otherwise. */
int
SbTuple_Unpack(SbObject *p, Sb_ssize_t count_min, Sb_ssize_t count_max, ...);

/* Return a tuple's size.
   WARNING: no type checks are performed. */
Sb_ssize_t
SbTuple_GetSizeUnsafe(SbObject *p);

/* Return an item at the given position.
   WARNING: no type/range checks are performed. */
SbObject *
SbTuple_GetItemUnsafe(SbObject *p, Sb_ssize_t pos);

/* Set the tuple's item at a given position.
   Note: This function "steals" a reference to `o`.
   WARNING: no type/range checks are performed. */
void
SbTuple_SetItemUnsafe(SbObject *p, Sb_ssize_t pos, SbObject *o);

/* Return a tuple's size.
   Returns: size if OK, -1 otherwise. */
Sb_ssize_t
SbTuple_GetSize(SbObject *p);

/* Return an item at the given position.
   Returns: Borrowed reference or NULL on failure. */
SbObject *
SbTuple_GetItem(SbObject *p, Sb_ssize_t pos);

/* Set the tuple's item at a given position.
   Note: This function "steals" a reference to `o`.
   Returns: 0 if OK, -1 otherwise. */
int
SbTuple_SetItem(SbObject *p, Sb_ssize_t pos, SbObject *o);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_TUPLE_H
