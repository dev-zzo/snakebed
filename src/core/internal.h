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

SbObject *
_SbErr_IncorrectSubscriptType(SbObject *sub);

SbObject *
_SbType_BuildMethodDict(const SbCMethodDef *methods);

SbTypeObject *
_SbType_FromCDefs(const char *name, SbTypeObject *base_type, const SbCMethodDef *methods, Sb_size_t basic_size);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_INTERNAL_H
