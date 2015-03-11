#include "snakebed.h"

SbObject *
_SbTuple_Prepend(SbObject *o, SbObject *tuple)
{
    SbObject *new_tuple;
    Sb_ssize_t pos, count;

    if (tuple) {
        count = SbTuple_GetSizeUnsafe(tuple);
        new_tuple = SbTuple_New(count + 1);
        if (!new_tuple) {
            return NULL;
        }
        Sb_INCREF(o);
        SbTuple_SetItemUnsafe(new_tuple, 0, o);
        for (pos = 0; pos < count; ++pos) {
            o = SbTuple_GetItemUnsafe(tuple, pos);
            Sb_INCREF(o);
            SbTuple_SetItemUnsafe(new_tuple, pos + 1, o);
        }
    }
    else {
        new_tuple = SbTuple_Pack(1, o);
        if (!new_tuple) {
            return NULL;
        }
        Sb_INCREF(o);
    }
    return new_tuple;
}

SbObject *
_SbErr_IncorrectSubscriptType(SbObject *sub)
{
    SbErr_RaiseWithFormat(SbErr_TypeError, "passed subscript type (%s) is not supported", Sb_TYPE(sub)->tp_name);
    return NULL;
}

