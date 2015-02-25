#include "snakebed.h"

/* Relying on compiler here. */
#include <stdarg.h>

/* Keep the type object here. */
SbTypeObject *SbTuple_Type = NULL;

/*
 * Internals
 */

static int
tuple_check_type_pos(SbObject *p, Sb_ssize_t pos)
{
    if (!SbTuple_CheckExact(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-tuple object passed to a tuple method");
        return -1;
    }
    /* Do an unsigned comparison. */
    if ((Sb_size_t)pos >= (Sb_size_t)SbTuple_GetSizeUnsafe(p)) {
        SbErr_RaiseWithString(SbErr_IndexError, "tuple index out of range");
        return -1;
    }
    return 0;
}

static void
tuple_destroy(SbTupleObject *self)
{
    Sb_ssize_t pos;
    Sb_ssize_t size = SbTuple_GetSizeUnsafe((SbObject *)self);

    for (pos = 0; pos < size; ++pos) {
        Sb_CLEAR(self->items[pos]);
    }
    SbObject_DefaultDestroy((SbObject *)self);
}

/*
 * C interface implementations
 */

SbObject *
SbTuple_New(Sb_ssize_t length)
{
    SbTupleObject *op;

    if (length < 0) {
        SbErr_RaiseWithString(SbErr_ValueError, "tuple length cannot be negative");
        return NULL;
    }

    /* Allocator returns the memory wiped with zeros, so no need to do that again. */
    op = (SbTupleObject *)SbObject_NewVar(SbTuple_Type, length);
    return (SbObject *)op;
}

SbObject *
SbTuple_PackVa(Sb_ssize_t count, va_list va)
{
    SbObject *tuple;
    Sb_ssize_t pos;

    tuple = SbTuple_New(count);
    if (!tuple) {
        return NULL;
    }

    for (pos = 0; pos < count; ++pos) {
        SbObject *o;

        o = va_arg(va, SbObject *);
        if (!o) {
            Sb_DECREF(tuple);
            SbErr_RaiseWithString(SbErr_ValueError, "a NULL pointer found when packing into a tuple");
            return NULL;
        }
        SbTuple_SetItemUnsafe(tuple, pos, o);
    }

    return tuple;
}

SbObject *
SbTuple_Pack(Sb_ssize_t count, ...)
{
    SbObject *tuple;
    va_list va;

    va_start(va, count);
    tuple = SbTuple_PackVa(count, va);
    va_end(va);
    return tuple;
}

int
SbTuple_Unpack(SbObject *p, Sb_ssize_t count_min, Sb_ssize_t count_max, ...)
{
    va_list va;
    Sb_ssize_t count, pos;

    if (!SbTuple_CheckExact(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-tuple object passed to a tuple method");
        return -1;
    }
    count = SbTuple_GetSizeUnsafe(p);
    if (count < count_min || count > count_max) {
        if (count_min < count_max) {
            SbErr_RaiseWithFormat(SbErr_TypeError, "tuple may contain between %d and %d items (%d given)", count_min, count_max, count);
        }
        else {
            SbErr_RaiseWithFormat(SbErr_TypeError, "tuple may contain exactly %d items (%d given)", count_min, count);
        }
        return -1;
    }

    va_start(va, count);
    for (pos = 0; pos < count; ++pos) {
        SbObject **po;

        po = va_arg(va, SbObject **);
        if (!po) {
            SbErr_RaiseWithString(SbErr_ValueError, "a NULL pointer found when unpacking a tuple");
            return -1;
        }
        *po = SbTuple_GetItemUnsafe(p, pos);
    }
    va_end(va);

    return 0;
}

Sb_ssize_t
SbTuple_GetSizeUnsafe(SbObject *p)
{
    return Sb_COUNT(p);
}

SbObject *
SbTuple_GetItemUnsafe(SbObject *p, Sb_ssize_t pos)
{
    SbTupleObject *op = (SbTupleObject *)p;
    return op->items[pos];
}

void
SbTuple_SetItemUnsafe(SbObject *p, Sb_ssize_t pos, SbObject *o)
{
    SbTupleObject *op = (SbTupleObject *)p;
    SbObject *oldp = op->items[pos];
    op->items[pos] = o;
    Sb_XDECREF(oldp);
}


Sb_ssize_t
SbTuple_GetSize(SbObject *p)
{
    if (!SbTuple_CheckExact(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-tuple object passed to a tuple method");
        return -1;
    }
    return SbTuple_GetSizeUnsafe(p);
}

SbObject *
SbTuple_GetItem(SbObject *p, Sb_ssize_t pos)
{
    if (tuple_check_type_pos(p, pos)) {
        return NULL;
    }

    return SbTuple_GetItemUnsafe(p, pos);
}

int
SbTuple_SetItem(SbObject *p, Sb_ssize_t pos, SbObject *o)
{
    if (tuple_check_type_pos(p, pos)) {
        return -1;
    }

    SbTuple_SetItemUnsafe(p, pos, o);
    return 0;
}

static SbObject *
tuple_len(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbInt_FromLong(SbTuple_GetSizeUnsafe(self));
}


/* Builtins initializer */
int
_SbTuple_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("tuple", NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbTupleObject) - sizeof(SbObject *);
    tp->tp_itemsize = sizeof(SbObject *);
    tp->tp_destroy = (destructor)tuple_destroy;

    SbTuple_Type = tp;
    return 0;
}

static const SbCMethodDef tuple_methods[] = {
    { "__len__", tuple_len },
    /* Sentinel */
    { NULL, NULL },
};

int
_SbTuple_BuiltinInit2()
{
    return SbType_CreateMethods(SbTuple_Type, tuple_methods);
}
