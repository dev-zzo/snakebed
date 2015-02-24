#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbMethod_Type = NULL;

/*
 * C interface implementations
 */

SbObject *
SbMethod_New(SbTypeObject *type, SbObject *func, SbObject *self)
{
    SbObject *p;

    p = SbObject_New(SbMethod_Type);
    if (p) {
        SbMethodObject *m = (SbMethodObject *)p;

        Sb_INCREF(type);
        m->type = type;
        Sb_INCREF(func);
        m->func = func;
        if (self) {
            Sb_INCREF(self);
            m->self = self;
        }
    }

    return p;
}

static void
method_destroy(SbMethodObject *self)
{
    Sb_CLEAR(self->type);
    Sb_CLEAR(self->func);
    Sb_CLEAR(self->self);
    SbObject_Destroy((SbObject *)self);
}

SbObject *
SbMethod_Call(SbObject *p, SbObject *args, SbObject *kwargs)
{
    SbMethodObject *m = (SbMethodObject *)p;
    SbObject *func;

    func = m->func;
    if (SbCFunction_Check(func)) {
        if (!m->self || m->self == Sb_None) {
            return SbCFunction_Call(func, NULL, args, kwargs);
        }
        else {
            return SbCFunction_Call(func, m->self, args, kwargs);
        }
    }
    return NULL;
}

/* Builtins initializer */

static const SbCMethodDef method_methods[] = {
    { "__call__", SbMethod_Call },

    /* Sentinel */
    { NULL, NULL },
};

int
_SbMethod_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("method", NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbMethodObject);
    tp->tp_destroy = (destructor)method_destroy;

    SbMethod_Type = tp;
    return SbType_CreateMethods(tp, method_methods);
}

