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

typedef SbObject *(*getattrfunc)(SbObject *self, char *key);
typedef SbObject *(*getattrofunc)(SbObject *self, SbObject *key);
typedef int (*setattrfunc)(SbObject *self, char *key, SbObject *value);
typedef int (*setattrofunc)(SbObject *self, SbObject *key, SbObject *value);

struct _SbTypeObject {
    SbObject_HEAD;

    struct _SbTypeObject *tp_base;

    /* Methods to implement standard operations */
    getattrofunc tp_getattro;
    setattrofunc tp_setattro;
    reprfunc tp_repr;
    reprfunc tp_str;
    hashfunc tp_hash;
};

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_TYPE_H
