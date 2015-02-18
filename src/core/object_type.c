#include "snakebed.h"
#include "object_type.h"

/* Keep the type object here. */
SbTypeObject *SbType_Type = NULL;

SbObject *
SbType_GenericAlloc(SbTypeObject *type, Sb_ssize_t nitems)
{
    SbObject *op;
    Sb_ssize_t size;

    size = type->tp_basicsize + nitems * type->tp_itemsize;
    /* TODO: check against overflows. */

    op = (SbObject *)SbObject_Malloc(size);
    if (!op) {
        /* OOM. */
        return NULL;
    }

    Sb_BZero(op, size);

    return op;
}

SbTypeObject *
SbType_New(const char *name, SbTypeObject *base_type)
{
    SbTypeObject *tp;

    tp = (SbTypeObject *)SbObject_New(SbType_Type);
    if (!tp) {
        return NULL;
    }

    tp->tp_name = name;
    tp->tp_alloc = SbType_GenericAlloc;
    tp->tp_free = SbObject_Free;

    /* If we have a base -- inherit what's inheritable. */
    if (base_type) {
        tp->tp_base = base_type;
        tp->tp_basicsize = base_type->tp_basicsize;
        tp->tp_itemsize = base_type->tp_itemsize;
        tp->tp_destroy = base_type->tp_destroy;
        tp->tp_hash = base_type->tp_hash;
    }

    return tp;
}

/* Handle destruction of type instances */
static void
type_destroy(SbTypeObject *tp)
{
    tp->tp_free(tp);
}

/* Builtins initializer */
int
_SbType_BuiltinInit()
{
    SbTypeObject *tp;
    Sb_size_t size = sizeof(SbTypeObject);

    tp = (SbTypeObject *)SbObject_Malloc(size);
    if (!tp) {
        /* OOM. */
        return -1;
    }

    Sb_BZero(tp, size);
    SbObject_INIT(tp, tp);

    tp->tp_name = "type";
    tp->tp_basicsize = size;
    tp->tp_destroy = (destructor)type_destroy;
    tp->tp_alloc = SbType_GenericAlloc;
    tp->tp_free = SbObject_Free;

    SbType_Type = tp;

    return 0;
}

