#ifndef __SNAKEBED_OBJECT_METHOD_H
#define __SNAKEBED_OBJECT_METHOD_H
#ifdef __cplusplus
extern "C" {
#endif

/* Define the C function object structure. */
typedef struct _SbMethodObject {
    SbObject_HEAD;
    SbTypeObject *type;
    SbObject *func;
    SbObject *self;
} SbMethodObject;

extern SbTypeObject *SbMethod_Type;

#define SbMethod_Check(p) \
    (Sb_TYPE(p) == SbMethod_Type)

SbObject *
SbMethod_New(SbTypeObject *type, SbObject *func, SbObject *self);

SbObject *
SbMethod_Call(SbObject *p, SbObject *args, SbObject *kwargs);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_METHOD_H
