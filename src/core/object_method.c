#include "snakebed.h"

/* Define the C function object structure. */
typedef struct _SbMethodObject {
    SbObject_HEAD;
} SbMethodObject;

/* Keep the type object here. */
SbTypeObject *SbMethod_Type = NULL;

/*
 * C interface implementations
 */

SbObject *
SbMethod_New(SbObject *type, SbObject *func)
{
    SbObject *p;

    p = SbObject_New(SbMethod_Type);
    if (p) {
        SbMethod_SetType(p, type);
        Sb_INCREF(func);
        SbMethod_SetFunc(p, func);
    }

    return p;
}

SbObject *
SbMethod_FromCFunction(SbObject *type, SbCFunction fp)
{
    SbObject *func;
    SbObject *p;

    func = SbCFunction_New(fp);
    if (!func) {
        return NULL;
    }

    p = SbMethod_New(type, func);
    if (!p) {
        Sb_DECREF(func);
    }
    return p;
}

SbObject *
SbMethod_Bind(SbObject *p, SbObject *self)
{
    SbObject *m;

    m = SbMethod_New(SbMethod_GetType(p), SbMethod_GetFunc(p));
    SbMethod_SetSelf(p, self);
    return m;
}

SbObject *
SbMethod_GetFunc(SbObject *p)
{
    return NULL;
}

SbObject *
SbMethod_GetType(SbObject *p)
{
    return NULL;
}

SbObject *
SbMethod_GetSelf(SbObject *p)
{
    return NULL;
}

int
SbMethod_SetFunc(SbObject *p, SbObject *o)
{
    return 0;
}

int
SbMethod_SetType(SbObject *p, SbObject *o)
{
    return 0;
}

int
SbMethod_SetSelf(SbObject *p, SbObject *o)
{
    return 0;
}

SbObject *
SbMethod_Call(SbObject *p, SbObject *args, SbObject *kwargs)
{
    SbObject *func;
    SbObject *self;

    func = SbMethod_GetFunc(p);
    self = SbMethod_GetSelf(p);
    if (SbCFunction_Check(func)) {
        if (!self || self == Sb_None) {
            return SbCFunction_Call(func, NULL, args, kwargs);
        }
        else {
            return SbCFunction_Call(func, self, args, kwargs);
        }
    }
    return NULL;
}

/* Builtins initializer */
int
_SbMethod_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("method", NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbMethodObject);
    /* Slots:
       __func__
       im_class
       __self__
    */
    tp->tp_flags = SbType_FLAGS_HAS_SLOTS;
    tp->tp_slotcount = 3;

    SbMethod_Type = tp;
    return 0;
}
