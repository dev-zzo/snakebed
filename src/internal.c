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

SbObject *
_SbType_BuildMethodDict(const SbCMethodDef *methods)
{
    SbObject *dict;
    SbObject *func;

    dict = SbDict_New();
    if (!dict) {
        goto fail0;
    }

    if (!methods) {
        return dict;
    }

    while (methods->name) {
        func = SbCFunction_New(methods->func);
        if (!func) {
            goto fail1;
        }
        if (SbDict_SetItemString(dict, methods->name, func) < 0) {
            goto fail1;
        }
        Sb_DECREF(func);
        methods++;
    }

    return dict;

fail1:
    Sb_DECREF(dict);
fail0:
    return NULL;
}

SbTypeObject *
_SbType_FromCDefs(const char *name, SbTypeObject *base_type, const SbCMethodDef *methods, Sb_size_t basic_size)
{
    SbTypeObject *tp;
    SbObject *dict;

    dict = _SbType_BuildMethodDict(methods);
    if (!dict) {
        return NULL;
    }

    tp = SbType_New(name, base_type, dict);
    Sb_DECREF(dict);
    if (!tp) {
        return NULL;
    }

    tp->tp_basicsize = basic_size;
    tp->tp_destroy = SbObject_DefaultDestroy;

    return tp;
}
