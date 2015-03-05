#include "snakebed.h"

SbObject *
_SbTuple_Prepend(SbObject *o, SbObject *tuple)
{
    SbObject *new_tuple;
    Sb_ssize_t pos, count;

    count = SbTuple_GetSizeUnsafe(tuple);
    new_tuple = SbTuple_New(count + 1);
    if (!new_tuple) {
        return NULL;
    }
    SbTuple_SetItemUnsafe(new_tuple, 0, o);
    for (pos = 0; pos < count; ++pos) {
        o = SbTuple_GetItemUnsafe(tuple, pos);
        Sb_INCREF(o);
        SbTuple_SetItemUnsafe(new_tuple, pos, o);
    }
    return new_tuple;
}

