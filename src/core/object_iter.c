#include "snakebed.h"

SbTypeObject *SbIter_Type = NULL;

SbObject *
SbIter_New(SbObject *o, SbObject *sentinel)
{
    SbObject *self;

    self = SbObject_New(SbIter_Type);
    if (self) {
        SbIterObject *myself = (SbIterObject *)self;

        Sb_INCREF(o);
        myself->iterable = o;
        if (sentinel) {
            Sb_INCREF(sentinel);
            myself->sentinel = sentinel;
        }
    }

    return self;
}

static void
iter_destroy(SbIterObject *self)
{
    Sb_CLEAR(self->sentinel);
    Sb_CLEAR(self->iterable);
    SbObject_DefaultDestroy((SbObject *)self);
}

static SbObject *
iter_new(SbObject *cls, SbObject *args, SbObject *kwargs)
{
    SbObject *result;
    SbTypeObject *o_type;
    SbObject *o = NULL, *sentinel = NULL;

    if (SbTuple_Unpack(args, 2, 3, &cls, &o, &sentinel) < 0) {
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
        result = SbIter_New(o, sentinel);
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
            result = SbIter_New(o, NULL);
            return result;
        }
        SbErr_RaiseWithFormat(SbErr_TypeError, "'%s' object is not iterable", o_type->tp_name);
        return NULL;
    }
}

static SbObject *
iter_next(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbIterObject *myself = (SbIterObject *)self;
    SbObject *result;

    if (myself->sentinel) {
        result = SbObject_Call(myself->iterable, NULL, NULL);
        if (!result) {
            return NULL;
        }
        if (result == myself->sentinel) {
            Sb_DECREF(result);
            return SbErr_NoMoreItems();
        }
    }
    else {
        result = SbObject_CallMethodObjArgs(myself->iterable, "__getitem__", 1, SbInt_FromLong(myself->index));
        ++myself->index;
        if (result) {
            return result;
        }
        if (SbErr_Occurred() && SbErr_ExceptionMatches(SbErr_Occurred(), (SbObject *)SbErr_IndexError)) {
            SbErr_Clear();
            return SbErr_NoMoreItems();
        }
    }
    return result;
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
