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
SbObject_DefaultDestroy(SbObject *p)
{
    Sb_TYPE(p)->tp_free(p);
}

SbObject *
SbObject_DefaultHash(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbInt_FromLong((long)self);
}

SbObject *
SbObject_DefaultSetAttr(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *attr_name;
    SbObject *value;

    if (SbTuple_Unpack(args, 2, 2, &attr_name, &value) < 0) {
        return NULL;
    }

    /* If the object has a dict, modify it. */
    if (Sb_TYPE(self)->tp_flags & SbType_FLAGS_HAS_DICT) {
        return SbDict_SetItemString(SbObject_DICT(self), attr_name, value);
    }
    return NULL;
}

SbObject *
SbObject_DefaultDelAttr(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *attr_name;

    if (SbTuple_Unpack(args, 1, 1, &attr_name) < 0) {
        return NULL;
    }

    /* If the object has a dict, modify it. */
    if (Sb_TYPE(self)->tp_flags & SbType_FLAGS_HAS_DICT) {
        return SbDict_DelItemString(SbObject_DICT(self), SbStr_AsString(attr_name));
    }
    return NULL;
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
    tp->tp_destroy = SbObject_DefaultDestroy;

    SbObject_Type = tp;
    return 0;
}
