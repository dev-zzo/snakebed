#include "snakebed.h"
#include "internal.h"

SbTypeObject *SbCode_Type;

SbObject *
SbCode_New(SbObject *name, long flags, long stack_size, long arg_count, SbObject *code, SbObject *consts, SbObject *names, SbObject *varnames)
{
    SbObject *self;

    self = SbObject_New(SbCode_Type);
    if (self) {
        SbCodeObject *myself = (SbCodeObject *)self;

        Sb_INCREF(name);
        myself->name = name;
        myself->flags = flags;
        myself->stack_size = stack_size;
        myself->arg_count = arg_count;
        Sb_INCREF(code);
        myself->code = code;
        Sb_INCREF(consts);
        myself->consts = consts;
        Sb_INCREF(names);
        myself->names = names;
        Sb_INCREF(varnames);
        myself->varnames = varnames;
    }
    return self;
}

static void
code_destroy(SbCodeObject *myself)
{
    Sb_XDECREF(myself->name);
    Sb_XDECREF(myself->code);
    Sb_CLEAR(myself->consts);
    Sb_XDECREF(myself->names);
    Sb_XDECREF(myself->varnames);
    SbObject_DefaultDestroy((SbObject *)myself);
}

/* Type initializer */

static const SbCMethodDef code_methods[] = {
    /* Sentinel */
    { NULL, NULL },
};

int
_SbCode_TypeInit()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("<code>", NULL, code_methods, sizeof(SbCodeObject));
    if (!tp) {
        return -1;
    }

    tp->tp_destroy = (SbDestroyFunc)code_destroy;

    SbCode_Type = tp;
    return 0;
}
