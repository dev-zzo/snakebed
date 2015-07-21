#include "snakebed.h"
#include "internal.h"

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
    SbObject_DefaultDestroy((SbObject *)self);
}

SbObject *
SbMethod_Call(SbObject *p, SbObject *args, SbObject *kwargs)
{
    SbMethodObject *m = (SbMethodObject *)p;
    SbObject *func;

    func = m->func;
    if (SbCFunction_Check(func)) {
        if (!m->self || m->self == Sb_None) {
            if (args) {
                Sb_ssize_t args_count;

                args_count = SbTuple_GetSize(args);
                if (args_count > 0) {
                    SbObject *new_args;
                    SbObject *result;
                    SbObject *self;
                    Sb_ssize_t pos;

                    /* Assume arg 1 is `self` and shift it */
                    new_args = SbTuple_New(args_count - 1);
                    if (!new_args) {
                        return NULL;
                    }
                    for (pos = 1; pos < args_count; ++pos) {
                        SbObject *o;

                        o = SbTuple_GetItemUnsafe(args, pos);
                        SbTuple_SetItemUnsafe(new_args, pos - 1, o);
                    }

                    self = SbTuple_GetItemUnsafe(args, 0);
                    result = SbCFunction_Call(func, self, new_args, kwargs);
                    Sb_DECREF(new_args);
                    return result;
                }
            }
            return SbCFunction_Call(func, NULL, args, kwargs);
        }
        else {
            return SbCFunction_Call(func, m->self, args, kwargs);
        }
    }
    if (SbPFunction_Check(func)) {
        if (!m->self || m->self == Sb_None) {
            return SbPFunction_Call(func, args, kwargs);
        }
        else {
            SbObject *new_args;
            SbObject *result;

            /* Inject `self` */
            new_args = _SbTuple_Prepend(m->self, args);
            if (!new_args) {
                return NULL;
            }

            result =  SbPFunction_Call(func, new_args, kwargs);
            Sb_DECREF(new_args);
            return result;
        }
    }
    SbErr_RaiseWithFormat(SbExc_SystemError, "method: got '%s' instead of function", Sb_TYPE(func)->tp_name);
    return NULL;
}

static SbObject *
method_getattr(SbMethodObject *self, SbObject *args, SbObject *kwargs)
{
    const char *attr_name;
    SbObject *value;

    if (SbArgs_Parse("s:name", args, kwargs, &attr_name) < 0) {
        return NULL;
    }
    if (!SbRT_StrCmp(attr_name, "__name__")) {
        return SbObject_GetAttrString(self->func, "__name__");
    }
    if (!SbRT_StrCmp(attr_name, "__func__")) {
        value = self->func;
        goto return_value;
    }
    if (!SbRT_StrCmp(attr_name, "__self__")) {
        value = self->self;
        goto return_value;
    }
    return SbObject_DefaultGetAttr((SbObject *)self, args, kwargs);

return_value:
    Sb_INCREF(value);
    return value;
}

/* Builtins initializer */

static const SbCMethodDef method_methods[] = {
    { "__call__", SbMethod_Call },
    { "__getattr__", (SbCFunction)method_getattr },

    /* Sentinel */
    { NULL, NULL },
};

int
_SbMethod_BuiltinInit()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("method", NULL, method_methods, sizeof(SbMethodObject));
    if (!tp) {
        return -1;
    }

    tp->tp_destroy = (SbDestroyFunc)method_destroy;

    SbMethod_Type = tp;
    return 0;
}

