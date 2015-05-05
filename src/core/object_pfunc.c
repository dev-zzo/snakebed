#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbPFunction_Type = NULL;

SbObject *
SbPFunction_New(SbCodeObject *code, SbObject *defaults, SbObject *globals)
{
    SbObject *p;

    if (!SbTuple_CheckExact(defaults)) {
        SbErr_RaiseWithString(SbErr_ValueError, "expected a tuple for `defaults`");
        return NULL;
    }
    if (!SbDict_CheckExact(globals)) {
        SbErr_RaiseWithString(SbErr_ValueError, "expected a dict for `globals`");
        return NULL;
    }

    p = (SbObject *)SbObject_New(SbPFunction_Type);
    if (p) {
        SbPFunctionObject *op = (SbPFunctionObject *)p;

        Sb_INCREF(code);
        op->code = code;
        Sb_INCREF(defaults);
        op->defaults = defaults;
        Sb_INCREF(globals);
        op->globals = globals;
    }
    return p;
}

static void
pfunction_destroy(SbPFunctionObject *self)
{
    Sb_CLEAR(self->defaults);
    Sb_CLEAR(self->globals);
    Sb_CLEAR(self->code);
    SbObject_DefaultDestroy((SbObject *)self);
}

SbObject *
SbPFunction_Call(SbObject *p, SbObject *args, SbObject *kwargs)
{
    SbFrameObject *frame;
    SbPFunctionObject *op = (SbPFunctionObject *)p;
    SbObject *locals = NULL;
    SbObject *result = NULL;

#if SUPPORTS_BUILTIN_TYPECHECKS
    if (!SbPFunction_Check(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-function object passed to a function method");
        goto fail0;
    }
#endif

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

    result = SbInterp_Execute(frame);
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

    tp = _SbType_FromCDefs("function", NULL, pfunction_methods, sizeof(SbPFunctionObject));
    if (!tp) {
        return -1;
    }

    tp->tp_destroy = (SbDestroyFunc)pfunction_destroy;

    SbPFunction_Type = tp;
    return 0;
}
