#include "snakebed.h"
#include "internal.h"

/* Keep the type object here. */
SbTypeObject *SbCFunction_Type = NULL;

/*
 * C interface implementations
 */

SbObject *
SbCFunction_New(SbCFunction fp)
{
    SbCFunctionObject *op;

    op = (SbCFunctionObject *)SbObject_New(SbCFunction_Type);
    if (op) {
        op->fp = fp;
    }

    return (SbObject *)op;
}

SbObject *
SbCFunction_Call(SbObject *p, SbObject *self, SbObject *args, SbObject *kwargs)
{
#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbCFunction_Check(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-cfunc object passed to a cfunc method");
        return NULL;
    }
#endif

    return ((SbCFunctionObject *)p)->fp(self, args, kwargs);
}

static SbObject *
cfunction_call(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return ((SbCFunctionObject *)self)->fp(NULL, args, kwargs);
}


/* Builtins initializer */

static const SbCMethodDef cfunc_methods[] = {
    { "__call__", cfunction_call },
    /* Sentinel */
    { NULL, NULL },
};

int
_Sb_TypeInit_CFunction()
{
    SbTypeObject *tp;

    tp = SbType_New("<C function>", NULL, NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbCFunctionObject);
    tp->tp_destroy = SbObject_DefaultDestroy;

    SbCFunction_Type = tp;
    return 0;
}

int
_Sb_TypeInit2_CFunction()
{
    SbObject *dict;

    dict = _SbType_BuildMethodDict(cfunc_methods);
    SbCFunction_Type->tp_dict = dict;
    return dict ? 0 : -1;
}
