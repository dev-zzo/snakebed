#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbPFunction_Type = NULL;

SbObject *
SbPFunction_New(SbCodeObject *code, SbObject *defaults, SbObject *globals)
{
    SbObject *p;

    p = (SbObject *)SbObject_NewVar(SbFrame_Type, code->stack_size);
    if (p) {
        SbPFunctionObject *op = (SbPFunctionObject *)p;

        Sb_INCREF(code);
        op->code = code;
        Sb_INCREF(defaults);
        op->defaults = defaults;
        Sb_INCREF(globals);
        op->globals = globals;
        op->name = SbStr_FromString("<unnamed function>");
    }
    return p;
}

static void
pfunction_destroy(SbPFunctionObject *self)
{
    Sb_CLEAR(self->name);
    Sb_CLEAR(self->defaults);
    Sb_CLEAR(self->globals);
    Sb_CLEAR(self->code);
    SbObject_DefaultDestroy((SbObject *)self);
}

SbObject *
SbPFunction_Call(SbObject *p, SbObject *args, SbObject *kwargs)
{
    SbObject *frame;
    SbPFunctionObject *op = (SbPFunctionObject *)p;
    SbObject *locals = NULL;
    SbObject *result = NULL;

    if (!SbPFunction_Check(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-function object passed to a function method");
        goto fail0;
    }

    if (op->code->flags & SbCode_NEWLOCALS) {
        locals = SbDict_New();
    }

    frame = SbFrame_New(op->code, op->globals, locals);
    Sb_XDECREF(locals);
    if (!frame) {
        goto fail0;
    }

    if (SbFrame_ApplyArgs(frame, args, kwargs, op->defaults) < 0) {
        goto fail1;
    }

    result = SbInterp_Execute((SbFrameObject *)frame);
    Sb_DECREF(frame);
    return result;

fail1:
    Sb_DECREF(frame);
fail0:
    return NULL;
}

/* Type initializer */

static const SbCMethodDef pfunction_methods[] = {
    { "__call__", SbPFunction_Call },

    /* Sentinel */
    { NULL, NULL },
};


int
_SbPFunction_TypeInit()
{
    SbTypeObject *tp;

    tp = SbType_New("function", NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbPFunctionObject);
    tp->tp_destroy = (destructor)pfunction_destroy;

    SbPFunction_Type = tp;
    return SbType_CreateMethods(SbPFunction_Type, pfunction_methods);
}