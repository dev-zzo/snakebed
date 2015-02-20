#include "snakebed.h"

/* Relying on compiler here. */
#include <stdarg.h>

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
        goto fail0;
    }

    Sb_BZero(op, size);
    /* Object fields get initialised afterwards */

    /* Check and allocate slots */
    if (type->tp_flags & SbType_FLAGS_HAS_SLOTS) {
    }
    else if (type->tp_flags & SbType_FLAGS_HAS_DICT) {
    }

    return op;

fail1:
    SbObject_Free(op);
fail0:
    return NULL;
}

static int
type_inherit(SbTypeObject *tp, SbTypeObject *base_type)
{
    tp->tp_base = base_type;
    tp->tp_basicsize = base_type->tp_basicsize;
    tp->tp_itemsize = base_type->tp_itemsize;
    tp->tp_destroy = base_type->tp_destroy;
    return 0;
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
        type_inherit(tp, base_type);
    }

    return tp;
}

/* Handle destruction of type instances */
static void
type_destroy(SbTypeObject *tp)
{
    if (tp->tp_flags & SbType_FLAGS_HAS_SLOTS) {
        Sb_DECREF(tp->tp_slotnames);
    }
    tp->tp_free(tp);
}

int
SbType_BuildSlots(SbTypeObject *type, Sb_ssize_t count, ...)
{
    SbObject *tuple;
    Sb_ssize_t pos;
    va_list args;

    tuple = SbTuple_New(count);
    if (!tuple) {
        goto fail0;
    }

    va_start(args, count);
    for (pos = 0; pos < count; ++pos) {
        SbObject *name;

        name = SbStr_FromString(va_arg(args, const char *));
        if (!name) {
            goto fail1;
        }
        SbTuple_SetItemUnsafe(tuple, pos, name);
    }
    va_end(args);

    type->tp_slotnames = tuple;
    return 0;

fail1:
    Sb_DECREF(tuple);
fail0:
    return -1;
}

SbObject *
_SbType_Lookup(SbObject *op, const char *name)
{
    SbTypeObject *type;

    type = Sb_TYPE(op);
    if (type->tp_flags & SbType_FLAGS_HAS_SLOTS) {
        Sb_ssize_t slot_index = 0;
        /* Walk the hierarchy */
        while (type) {
            Sb_ssize_t slot_count;
            Sb_ssize_t intype_index;
            SbObject *slot_names;

            /* TODO: This assumes slot names are always a tuple. */
            slot_names = type->tp_slotnames;
            slot_count = SbTuple_GetSizeUnsafe(slot_names);
            for (intype_index = 0; intype_index < slot_count; ++intype_index, ++slot_index) {
                if (_SbStr_EqString(SbTuple_GetItemUnsafe(slot_names, intype_index), name)) {

                }
            }
            type = type->tp_base;
        }
        return NULL;
    }
    else if (type->tp_flags & SbType_FLAGS_HAS_DICT) {
        SbObject *item;

        /* Check instance dictionary first */
        item = SbDict_GetItemString(SbObject_DICT(op), name);
        if (item) {
            return item;
        }
    }

    return SbDict_GetItemString(SbObject_DICT(type), name);
}

SbObject *
_SbType_FindMethod(SbObject *op, const char *name)
{
    SbObject *attr;

    attr = _SbType_Lookup(op, name);
    if (attr && SbMethod_Check(attr)) {
        attr = SbMethod_Bind(attr, op);
    }
    return attr;
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

int
_SbType_BuiltinInit2()
{
    /* Types have a dict by default. */
    return 0;
}
