#include "snakebed.h"

/* Relying on compiler here. */
#include <stdarg.h>

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
        new_items = SbObject_Realloc(self->items, new_allocated * sizeof(SbObject *));
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

static int
list_check_type_pos(SbObject *p, Sb_ssize_t pos)
{
    if (!SbList_CheckExact(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-list object passed to a list method");
        return -1;
    }
    /* Do an unsigned comparison. */
    if ((Sb_size_t)pos >= (Sb_size_t)SbList_GetSizeUnsafe(p)) {
        SbErr_RaiseWithString(SbErr_IndexError, "list index out of range");
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
        SbErr_RaiseWithString(SbErr_ValueError, "list length cannot be negative");
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
                SbErr_RaiseWithString(SbErr_ValueError, "a NULL pointer found when packing into a list");
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
    if (!SbList_CheckExact(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-list object passed to a list method");
        return -1;
    }
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

    if (!SbList_CheckExact(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-list object passed to a list method");
        goto fail0;
    }

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


/* Builtins initializer */
int
_SbList_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("list", NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbListObject);
    tp->tp_destroy = (SbDestroyFunc)list_destroy;

    SbList_Type = tp;
    return 0;
}
