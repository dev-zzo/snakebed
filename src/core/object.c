#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbObject_Type = NULL;

void _SbObject_DecRef(SbObject *op)
{
    Sb_ssize_t new_refcount;

    new_refcount = --op->ob_refcount;
    if (new_refcount) {
        return;
    }
    Sb_TYPE(op)->tp_destroy(op);
    /* `op` becomes invalid after this point. */
}

SbObject *
SbObject_New(SbTypeObject *type)
{
    SbObject *op;
    op = (SbObject *)type->tp_alloc(type, 0);
    SbObject_INIT(op, type);
    return op;
}

SbVarObject *
SbObject_NewVar(SbTypeObject *type, Sb_ssize_t count)
{
    SbVarObject *op;
    op = (SbVarObject *)type->tp_alloc(type, count);
    SbObject_INIT_VAR(op, type, count);
    return op;
}

void
SbObject_Destroy(SbObject *p)
{
    Sb_TYPE(p)->tp_free(p);
}

/* Python accessible methods */

static long
object_hash(SbObject *self)
{
    return (long)self;
}

/* Builtins initializer */
int
_SbObject_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("object", NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbObject);
    tp->tp_flags = SbType_FLAGS_HAS_DICT;
    tp->tp_destroy = SbObject_Destroy;

    SbObject_Type = tp;
    return 0;
}
