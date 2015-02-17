#include "snakebed.h"
#include "object_tuple.h"
#include "object_type.h"

/* Define the Tuple object structure. */
typedef struct _SbTupleObject {
    SbObject_HEAD;
    SbObject *items[1];
} SbTupleObject;

/* Keep the type object here. */
SbTypeObject *SbTuple_Type = NULL;

/*
 * C interface implementations
 */

int
SbTuple_CheckExact(SbObject *op)
{
    return Sb_TYPE(op) == SbTuple_Type;
}

SbObject *
SbTuple_New(Sb_ssize_t length)
{
    SbTupleObject *op;

    if (length < 0) {
        /* Raise ArgumentError */
        return NULL;
    }

    /* Allocator returns the memory wiped with zeros, so no need to do that again. */
    op = (SbTupleObject *)SbObject_NewVar(SbTuple_Type, length);
    return (SbObject *)op;
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
    Sb_CLEAR(op->items[pos]);
    op->items[pos] = o;
}


Sb_ssize_t
SbTuple_GetSize(SbObject *p)
{
    if (!SbTuple_CheckExact(p)) {
        /* raise TypeError? */
        return -1;
    }
    return SbTuple_GetSizeUnsafe(p);
}

static int
tuple_check_type_pos(SbObject *p, Sb_ssize_t pos)
{
    if (!SbTuple_CheckExact(p)) {
        /* raise TypeError? */
        return -1;
    }
    if (pos < 0 || pos >= SbTuple_GetSizeUnsafe(p)) {
        /* raise IndexError */
        return -1;
    }
    return 0;
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


SbObject *
SbTuple_GetSlice(SbObject *p, Sb_ssize_t low, Sb_ssize_t high)
{
    return NULL;
}


static void
tuple_destroy(SbTupleObject *self)
{
    Sb_ssize_t pos;
    Sb_ssize_t size = SbTuple_GetSizeUnsafe((SbObject *)self);

    for (pos = 0; pos < size; ++pos) {
        Sb_CLEAR(self->items[pos]);
    }
}


/* Builtins initializer */
int
_SbTuple_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("tuple", SbObject_Type);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbTupleObject) - sizeof(SbObject *);
    tp->tp_itemsize = sizeof(SbObject *);
    tp->tp_destroy = (destructor)tuple_destroy;

    SbTuple_Type = tp;
    return 0;
}
