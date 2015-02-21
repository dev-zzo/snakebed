#include "snakebed.h"

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

static void
cfunction_destroy(SbObject *p)
{
    SbObject_Destroy(p);
}

SbObject *
SbCFunction_Call(SbObject *p, SbObject *self, SbObject *args, SbObject *kwargs)
{
    if (!SbCFunction_Check(p)) {
        /* raise something? */
        goto fail0;
    }

    return ((SbCFunctionObject *)p)->fp(self, args, kwargs);

fail0:
    return NULL;
}


/* Builtins initializer */
int
_SbCFunction_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("<C function>", NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbCFunctionObject);
    tp->tp_destroy = cfunction_destroy;

    SbCFunction_Type = tp;
    return 0;
}

int
_SbCFunction_BuiltinInit2()
{
    return SbType_BuildSlots(SbCFunction_Type, 0);
}
