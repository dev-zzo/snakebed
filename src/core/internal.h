#ifndef __SNAKEBED_INTERNAL_H
#define __SNAKEBED_INTERNAL_H
#ifdef __cplusplus
extern "C" {
#endif

/* Assorted internal functions which are not worthy to be included in public API */

/* Make a new tuple from `o` and `tuple` items.
   Returns: New reference. */
SbObject *
_SbTuple_Prepend(SbObject *o, SbObject *tuple);


#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_INTERNAL_H
