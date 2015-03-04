#ifndef __SNAKEBED_OBJECT_TYPE_H
#define __SNAKEBED_OBJECT_TYPE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"

/* SbTypeObject's tp_alloc, tp_free, and tp_destroy */
typedef SbObject *(*SbAllocFunc)(SbTypeObject *self, Sb_ssize_t nitems);
typedef void (*SbDestroyFunc)(SbObject *self);
typedef void (*SbFreeFunc)(void *);

struct _SbTypeObject {
    SbObject_HEAD;

    /* In format "<module>.<name>" */ 
    const char *tp_name;

    /* Instance's basic size */
    size_t tp_basicsize;
    /* Instance's item size (for var-objects) */
    size_t tp_itemsize;

    long tp_flags;

    /* Base type */
    SbTypeObject *tp_base;

    /* Offset of the dictionary pointer in the instance. */
    Sb_ssize_t tp_dictoffset;

    /* Methods that are not available from Python */

    /* A pointer to the instance SbDestroyFunc function. The SbDestroyFunc function should:
       free all references which the instance owns, 
       free all memory buffers owned by the instance, 
       and finally (as its last action) call the type's tp_free function.
    */
    SbDestroyFunc tp_destroy;

    /* An optional pointer to an instance allocation function. */
    SbAllocFunc tp_alloc;
    /* An optional pointer to an instance deallocation function. */
    SbFreeFunc tp_free;

    /* Type object instance's dict. */
    SbObject *tp_dict;
};

extern SbTypeObject *SbType_Type;

enum {
    SbType_FLAGS_HAS_DICT           = (1 << 2),
};

#define SbType_Check(p) \
    (Sb_TYPE(p) == SbType_Type)

SbObject *
SbType_GenericAlloc(SbTypeObject *type, Sb_ssize_t nitems);

/* Generic implementation of the `__new__` method.
   Do NOT call from C code directly. */
SbObject *
SbType_GenericNew(SbObject *dummy, SbObject *args, SbObject *kwds);

/* Create a new type object.
   Returns: New reference. */
SbTypeObject *
SbType_New(const char *name, SbTypeObject *base_type);

/* Check whether `a` is a subtype of `b`. */
int
SbType_IsSubtype(SbTypeObject *a, SbTypeObject *b);

#define SbObject_DICT(p) \
    (*(SbObject **)(((char *)(p)) + Sb_TYPE(p)->tp_dictoffset))

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_TYPE_H
