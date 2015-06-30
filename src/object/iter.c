#include "snakebed.h"
#include "internal.h"

SbTypeObject *SbIter_Type = NULL;

static SbObject *
iter_next_iterable(SbIterObject *myself)
{
    SbObject *result;

    result = SbSequence_GetItem(myself->u.with_iterable.iterable, myself->index);
    ++myself->index;
    if (result) {
        Sb_INCREF(result);
        return result;
    }
    if (SbErr_Occurred() && SbErr_ExceptionMatches(SbErr_Occurred(), (SbObject *)SbErr_IndexError)) {
        SbErr_Clear();
    }
    return NULL;
}

static void
iter_cleanup_iterable(SbIterObject *myself)
{
    Sb_CLEAR(myself->u.with_iterable.iterable);
}

SbObject *
SbIter_New(SbObject *o)
{
    SbObject *self;

    self = SbObject_New(SbIter_Type);
    if (self) {
        SbIterObject *myself = (SbIterObject *)self;

        myself->cleanupproc = &iter_cleanup_iterable;
        myself->nextproc = &iter_next_iterable;
        Sb_INCREF(o);
        myself->u.with_iterable.iterable = o;
        myself->u.with_iterable.index = 0;
    }

    return self;
}

static SbObject *
iter_next_sentinel(SbIterObject *myself)
{
    SbObject *result;

    result = SbObject_Call(myself->u.with_sentinel.iterable, NULL, NULL);
    if (!result) {
        return NULL;
    }
    if (result == myself->u.with_sentinel.sentinel) {
        Sb_DECREF(result);
        return NULL;
    }
    return result;
}

static void
iter_cleanup_sentinel(SbIterObject *myself)
{
    Sb_CLEAR(myself->u.with_sentinel.iterable);
    Sb_CLEAR(myself->u.with_sentinel.sentinel);
}

SbObject *
SbIter_New2(SbObject *o, SbObject *sentinel)
{
    SbObject *self;

    self = SbObject_New(SbIter_Type);
    if (self) {
        SbIterObject *myself = (SbIterObject *)self;

        myself->cleanupproc = &iter_cleanup_sentinel;
        myself->nextproc = &iter_next_sentinel;
        Sb_INCREF(o);
        myself->u.with_sentinel.iterable = o;
        Sb_INCREF(sentinel);
        myself->u.with_sentinel.sentinel = sentinel;
    }

    return self;
}

static SbObject *
iter_next_array(SbIterObject *myself)
{
    SbObject **cursor;
    SbObject *result;

    cursor = myself->u.with_array.cursor;
    if (cursor >= myself->u.with_array.end) {
        return NULL;
    }
    myself->u.with_array.cursor = cursor + 1;
    result = *cursor;
    Sb_INCREF(result);
    return result;
}

static void
iter_cleanup_array(SbIterObject *myself)
{
    /* Nothing to do */
}

SbObject *
SbArrayIter_New(SbObject **base, SbObject **end)
{
    SbObject *self;

    self = SbObject_New(SbIter_Type);
    if (self) {
        SbIterObject *myself = (SbIterObject *)self;

        myself->cleanupproc = &iter_cleanup_array;
        myself->nextproc = &iter_next_array;
        myself->u.with_array.cursor = base;
        myself->u.with_array.end = end;
    }

    return self;
}

static void
iter_destroy(SbIterObject *self)
{
    self->cleanupproc(self);
    SbObject_DefaultDestroy((SbObject *)self);
}

static SbObject *
iter_new(SbObject *cls, SbObject *args, SbObject *kwargs)
{
    SbObject *result;
    SbTypeObject *o_type;
    SbObject *o = NULL, *sentinel = NULL;

    if (SbArgs_Unpack(args, 2, 3, &cls, &o, &sentinel) < 0) {
        return NULL;
    }

    o_type = Sb_TYPE(o);
    /* https://docs.python.org/2/library/functions.html#iter */
    if (sentinel) {
        /* If the second argument, sentinel, is given, then o must be a callable object. */
        if (!SbDict_GetItemString(o_type->tp_dict, "__call__")) {
            SbErr_RaiseWithFormat(SbErr_TypeError, "'%s' object is not callable", o_type->tp_name);
            return NULL;
        }
        result = SbIter_New2(o, sentinel);
        return result;
    }
    else {
        /* Without a second argument, o must be a collection object which 
         * supports the iteration protocol (the __iter__() method), or 
         * it must support the sequence protocol (the __getitem__() method). */
        if (SbDict_GetItemString(o_type->tp_dict, "__iter__")) {
            /* Simply pass the object's iterator */
            return SbObject_CallMethod(o, "__iter__", NULL, NULL);
        }
        if (SbDict_GetItemString(o_type->tp_dict, "__getitem__")) {
            result = SbIter_New(o);
            return result;
        }
        SbErr_RaiseWithFormat(SbErr_TypeError, "'%s' object is not iterable", o_type->tp_name);
        return NULL;
    }
}

static SbObject *
iter_next(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *result;
    SbIterObject *myself = (SbIterObject *)self;

    result = myself->nextproc(myself);
    if (result) {
        return result;
    }

    if (!SbErr_Occurred()) {
        return SbErr_NoMoreItems();
    }
    return NULL;
}


/* Type initializer */

static const SbCMethodDef iter_methods[] = {
    { "__new__", (SbCFunction)iter_new },
    { "next", (SbCFunction)iter_next },
    /* Sentinel */
    { NULL, NULL },
};

int
_Sb_TypeInit_Iter()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("iter", NULL, iter_methods, sizeof(SbIterObject));
    if (!tp) {
        return -1;
    }
    tp->tp_destroy = (SbDestroyFunc)iter_destroy;
    SbIter_Type = tp;
    return 0;
}


SbObject *
SbIter_Next(SbObject *o)
{
    SbObject *r;

    r = SbObject_CallMethod(o, "next", NULL, NULL);
    if (!r) {
        if (SbErr_Occurred() && SbErr_ExceptionMatches(SbErr_Occurred(), (SbObject *)SbErr_StopIteration)) {
            SbErr_Clear();
        }
    }
    return r;
}
