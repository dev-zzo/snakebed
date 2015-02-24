#ifndef __SNAKEBED_OBJECT_CFUNC_H
#define __SNAKEBED_OBJECT_CFUNC_H
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"

typedef SbObject *(*SbCFunction)(SbObject *self, SbObject *args, SbObject *kwargs);

/* Define the C function object structure. */
typedef struct _SbCFunctionObject {
    SbObject_HEAD;
    SbCFunction fp;
} SbCFunctionObject;

extern SbTypeObject *SbCFunction_Type;

#define SbCFunction_Check(p) \
    (Sb_TYPE(p) == SbCFunction_Type)

SbObject *
SbCFunction_New(SbCFunction fp);

SbObject *
SbCFunction_Call(SbObject *p, SbObject *self, SbObject *args, SbObject *kwargs);

/* Define a method implemented by a C function. */
typedef struct {
    const char *name;
    SbCFunction func;
} SbCMethodDef;

int
SbType_CreateMethods(SbTypeObject *type, const SbCMethodDef *methods);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_CFUNC_H
