#include "snakebed.h"
#include "internal.h"

/* Keep the type object here. */
SbTypeObject *SbList_Type = NULL;

static int
list_resize(SbListObject *self, Sb_ssize_t new_length)
{
    Sb_ssize_t old_allocated = self->allocated;
    if (old_allocated < new_length) {
        Sb_ssize_t new_allocated;
        SbObject **new_items;
        
        new_allocated = new_length + 4;
        new_items = (SbObject **)SbObject_Realloc(self->items, new_allocated * sizeof(SbObject *));
        if (!new_items) {
            /* OOM */
            SbErr_NoMemory();
            return -1;
        }

        while (old_allocated < new_allocated) {
            new_items[old_allocated++] = NULL;
        }

        self->items = new_items;
        self->allocated = new_allocated;
    }
    self->count = new_length;
    return 0;
}

static void
list_compact(SbObject *self, SbInt_Native_t offset)
{
    SbListObject * const myself = (SbListObject *)self;
    SbObject **dst, **src;
    SbObject **items;
    SbObject **limit;

    items = myself->items;
    limit = items + myself->count;
    dst = src = items + offset;
    while (src != limit) {
        if (*src) {
            *dst = *src;
            ++dst;
        }
        ++src;
    }
    myself->count = dst - items;
}

static int
list_check_type_pos(SbObject *p, Sb_ssize_t pos)
{
#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbList_CheckExact(p)) {
        SbErr_RaiseWithString(SbExc_SystemError, "non-list object passed to a list method");
        return -1;
    }
#endif
    /* Do an unsigned comparison. */
    if ((Sb_size_t)pos >= (Sb_size_t)SbList_GetSizeUnsafe(p)) {
        SbErr_RaiseWithString(SbExc_IndexError, "list index out of range");
        return -1;
    }
    return 0;
}

static void
list_destroy(SbListObject *self)
{
    Sb_ssize_t pos;
    Sb_ssize_t size = SbList_GetSizeUnsafe(self);

    for (pos = 0; pos < size; ++pos) {
        Sb_CLEAR(self->items[pos]);
    }
    SbObject_Free(self->items);
    SbObject_DefaultDestroy((SbObject *)self);
}

/*
 * C interface implementations
 */

int
SbList_CheckExact(SbObject *op)
{
    return Sb_TYPE(op) == SbList_Type;
}

SbObject *
SbList_New(Sb_ssize_t length)
{
    SbListObject *op;

    if (length < 0) {
        SbErr_RaiseWithString(SbExc_ValueError, "list length cannot be negative");
        goto fail0;
    }

    /* Allocator returns the memory wiped with zeros, so no need to do that again. */
    op = (SbListObject *)SbObject_New(SbList_Type);
    if (op) {
        if (list_resize(op, length) < 0) {
            goto fail1;
        }
    }
    return (SbObject *)op;

fail1:
    Sb_DECREF(op);
fail0:
    return NULL;
}

SbObject *
SbList_Pack(Sb_ssize_t count, ...)
{
    SbObject *list;

    list = SbList_New(count);
    if (list) {
        va_list args;
        Sb_ssize_t pos;

        va_start(args, count);
        for (pos = 0; pos < count; ++pos) {
            SbObject *o;

            o = va_arg(args, SbObject *);
            if (!o) {
                Sb_DECREF(list);
                SbErr_RaiseWithString(SbExc_ValueError, "a NULL pointer found when packing into a list");
                return NULL;
            }
            SbList_SetItemUnsafe(list, pos, o);
        }
        va_end(args);
    }

    return list;
}


Sb_ssize_t
SbList_GetSize(SbObject *p)
{
#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbList_CheckExact(p)) {
        SbErr_RaiseWithString(SbExc_SystemError, "non-list object passed to a list method");
        return -1;
    }
#endif

    return SbList_GetSizeUnsafe(p);
}

SbObject *
SbList_GetItem(SbObject *p, Sb_ssize_t pos)
{
    if (list_check_type_pos(p, pos)) {
        return NULL;
    }

    return SbList_GetItemUnsafe(p, pos);
}

int
SbList_SetItem(SbObject *p, Sb_ssize_t pos, SbObject *o)
{
    SbListObject *op = (SbListObject *)p;
    SbObject *oldp;

    if (list_check_type_pos(p, pos)) {
        return -1;
    }

    oldp = SbList_GetItemUnsafe(p, pos);
    SbList_SetItemUnsafe(p, pos, o);
    Sb_XDECREF(oldp);
    return 0;
}

int
SbList_Append(SbObject *p, SbObject *o)
{
    Sb_ssize_t pos;

#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbList_CheckExact(p)) {
        SbErr_RaiseWithString(SbExc_SystemError, "non-list object passed to a list method");
        goto fail0;
    }
#endif

    pos = SbList_GetSizeUnsafe(p);
    if (list_resize((SbListObject *)p, pos + 1) < 0) {
        goto fail0;
    }

    Sb_INCREF(o);
    SbList_SetItemUnsafe(p, pos, o);
    return 0;

fail0:
    return -1;
}


static SbObject *
list_len(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbInt_FromNative(SbList_GetSizeUnsafe(self));
}

static SbObject *
list_getitem(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *index;
    SbObject *result;

    if (SbArgs_Unpack(args, 1, 1, &index) < 0) {
        return NULL;
    }
    if (SbSlice_Check(index)) {
        SbInt_Native_t start, end, step, slice_length;
        SbInt_Native_t my_pos, result_pos;

        if (SbSlice_GetIndices(index, SbList_GetSizeUnsafe(self), &start, &end, &step, &slice_length) < 0) {
            return NULL;
        }

        result = SbList_New(slice_length);
        if (!result) {
            return NULL;
        }

        result_pos = 0;
        my_pos = start;
        while (my_pos < end) {
            SbList_SetItemUnsafe(result, result_pos, SbList_GetItemUnsafe(self, my_pos));
            result_pos += 1;
            my_pos += step;
        }

        return result;
    }
    if (SbInt_Check(index)) {
        result = SbList_GetItem(self, SbInt_AsNativeUnsafe(index));
        if (result) {
            Sb_INCREF(result);
        }
        return result;
    }
    return _SbErr_IncorrectSubscriptType(index);
}

static SbObject *
list_setitem(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *index;
    SbObject *value;

    if (SbArgs_Unpack(args, 2, 2, &index, &value) < 0) {
        return NULL;
    }
    if (SbSlice_Check(index)) {
        SbInt_Native_t start, end, step, slice_length;
        SbInt_Native_t my_pos;
        SbObject *it;

        if (SbSlice_GetIndices(index, SbList_GetSizeUnsafe(self), &start, &end, &step, &slice_length) < 0) {
            return NULL;
        }

        it = SbObject_GetIter(value);
        if (!it) {
            return NULL;
        }

        my_pos = start;
        while (my_pos < end) {
            SbObject *o;

            o = SbIter_Next(it);
            if (!o) {
                if (SbErr_Occurred()) {
                    return NULL;
                }
                /* TODO: CPython starts appending items here. */
                break;
            }
            SbList_SetItemUnsafe(self, my_pos, o);
            my_pos += step;
        }

        Sb_RETURN_NONE;
    }
    if (SbInt_Check(index)) {
        int result;

        Sb_INCREF(value);
        result = SbList_SetItem(self, SbInt_AsNativeUnsafe(index), value);
        if (result < 0) {
            return NULL;
        }
        Sb_RETURN_NONE;
    }
    return _SbErr_IncorrectSubscriptType(index);
}

static SbObject *
list_delitem(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *index;

    if (SbArgs_Unpack(args, 1, 1, &index) < 0) {
        return NULL;
    }
    if (SbSlice_Check(index)) {
        SbInt_Native_t start, end, step, slice_length;
        SbInt_Native_t my_pos;

        if (SbSlice_GetIndices(index, SbList_GetSizeUnsafe(self), &start, &end, &step, &slice_length) < 0) {
            return NULL;
        }

        my_pos = start;
        while (my_pos < end) {
            SbList_SetItemUnsafe(self, my_pos, NULL);
            my_pos += step;
        }

        list_compact(self, start);
        Sb_RETURN_NONE;
    }
    if (SbInt_Check(index)) {
        SbInt_Native_t my_pos;
        int result;

        my_pos = SbInt_AsNativeUnsafe(index);
        result = SbList_SetItem(self, my_pos, NULL);
        if (result < 0) {
            return NULL;
        }
        list_compact(self, my_pos);
        Sb_RETURN_NONE;
    }
    return _SbErr_IncorrectSubscriptType(index);
}

static SbObject *
list_iter(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject **base;

    base = ((SbListObject *)self)->items;
    return SbArrayIter_New(base, base + SbList_GetSizeUnsafe(self));
}


static SbObject *
list_append(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o;

    if (SbArgs_Unpack(args, 1, 1, &o) < 0) {
        return NULL;
    }

    if (SbList_Append(self, o) < 0) {
        return NULL;
    }
    Sb_RETURN_NONE;
}

/* Type initializer */

static const SbCMethodDef list_methods[] = {
    { "__len__", list_len },
    { "__getitem__", list_getitem },
    { "__setitem__", list_setitem },
    { "__delitem__", list_delitem },
    { "__iter__", list_iter },

    { "append", list_append },
    /* Sentinel */
    { NULL, NULL },
};

int
_Sb_TypeInit_List()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("list", NULL, list_methods, sizeof(SbListObject));
    if (!tp) {
        return -1;
    }

    tp->tp_destroy = (SbDestroyFunc)list_destroy;

    SbList_Type = tp;
    return 0;
}
