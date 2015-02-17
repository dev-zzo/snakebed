#ifndef __SNAKEBED_OBJECT_TYPE_H
#define __SNAKEBED_OBJECT_TYPE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"

typedef SbObject *(*unaryfunc)(SbObject *self);
typedef SbObject *(*binaryfunc)(SbObject *self, SbObject *);
typedef SbObject *(*ternaryfunc)(SbObject *self, SbObject *, SbObject *);

/* __repr__() and __str__() */
typedef SbObject *(*reprfunc)(SbObject *self);
/* __hash__() */
typedef long (*hashfunc)(SbObject *self);
/* */
typedef SbObject *(*richcmpfunc) (SbObject *self, SbObject *other, int op);

typedef SbObject *(*getattrofunc)(SbObject *self, SbObject *key);
typedef int (*setattrofunc)(SbObject *self, SbObject *key, SbObject *value);

typedef SbObject *(*getiterfunc)(SbObject *self);
typedef SbObject *(*iternextfunc) (SbObject *self);

typedef SbObject *(*allocfunc)(SbTypeObject *self, Sb_ssize_t nitems);
typedef SbObject *(*newfunc)(SbTypeObject *subtype, SbObject *args, SbObject *kwds);
typedef int (*initfunc)(SbObject *self, SbObject *args, SbObject *kwds);
typedef void (*destructor)(SbObject *self);
typedef void (*freefunc)(void *);

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

    /* Methods that are not available from Python */

    /* A pointer to the instance destructor function. The destructor function should:
       free all references which the instance owns, 
       free all memory buffers owned by the instance, 
       and finally (as its last action) call the type's tp_free function.
    */
    destructor tp_destroy;

    /* An optional pointer to an instance allocation function. */
    allocfunc tp_alloc;
    /* An optional pointer to an instance deallocation function. */
    freefunc tp_free;

    /* Methods that implement certain Python accessible methods */

    hashfunc tp_hash;
};

extern SbTypeObject *SbType_Type;

SbObject *
SbType_GenericAlloc(SbTypeObject *type, Sb_ssize_t nitems);

SbObject *
SbType_GenericNew(SbTypeObject *type, SbObject *args, SbObject *kwds);

/* Create a new type object.
   Return value: New reference.
 */
SbTypeObject *
SbType_New(const char *name, SbTypeObject *base_type);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_TYPE_H
