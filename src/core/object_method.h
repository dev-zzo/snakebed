#ifndef __SNAKEBED_OBJECT_METHOD_H
#define __SNAKEBED_OBJECT_METHOD_H
#ifdef __cplusplus
extern "C" {
#endif

extern SbTypeObject *SbMethod_Type;

#define SbMethod_Check(p) \
    (Sb_TYPE(p) == SbMethod_Type)

SbObject *
SbMethod_New(SbObject *type, SbObject *func);

SbObject *
SbMethod_FromCFunction(SbObject *type, SbCFunction fp);

SbObject *
SbMethod_Bind(SbObject *p, SbObject *self);

SbObject *
SbMethod_GetFunc(SbObject *p);
SbObject *
SbMethod_GetType(SbObject *p);
SbObject *
SbMethod_GetSelf(SbObject *p);
int
SbMethod_SetFunc(SbObject *p, SbObject *o);
int
SbMethod_SetType(SbObject *p, SbObject *o);
int
SbMethod_SetSelf(SbObject *p, SbObject *o);

SbObject *
SbMethod_Call(SbObject *p, SbObject *args, SbObject *kwargs);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_METHOD_H
