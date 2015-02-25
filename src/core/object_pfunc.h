#ifndef __SNAKEBED_OBJECT_PFUNC_H
#define __SNAKEBED_OBJECT_PFUNC_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SbPFunctionObject {
    SbObject_HEAD;
    SbCodeObject *code;
    SbObject *globals; /* dict */
    SbObject *defaults; /* tuple */
    SbObject *name; /* str */
} SbPFunctionObject;

extern SbTypeObject *SbPFunction_Type;

SbObject *
SbPFunction_New(SbCodeObject *code, SbObject *defaults, SbObject *globals);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_PFUNC_H
