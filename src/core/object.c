#include "snakebed.h"
#include "object.h"
#include "object_type.h"

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

/* Python accessible methods */

static long
object_hash(SbObject *self)
{
    return (long)self;
}

/* Type internals */

static void
object_destroy(SbObject *self)
{
    Sb_TYPE(self)->tp_free(self);
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
    tp->tp_destroy = (destructor)object_destroy;
    tp->tp_hash = (hashfunc)object_hash;

    SbObject_Type = tp;
    return 0;
}
