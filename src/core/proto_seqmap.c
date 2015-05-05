#include "snakebed.h"

Sb_ssize_t
SbObject_GetSize(SbObject *o)
{
    SbObject *result;

    result = SbObject_CallMethod(o, "__len__", NULL, NULL);
    if (result && SbInt_CheckExact(result)) {
        return SbInt_AsNative(result);
    }
    return -1;
}

SbObject *
SbObject_GetItem(SbObject *o, SbObject *key)
{
    return SbObject_CallMethodObjArgs(o, "__getitem__", 1, key);
}

int
SbObject_SetItem(SbObject *o, SbObject *key, SbObject *value)
{
    SbObject *result;

    result = SbObject_CallMethodObjArgs(o, "__setitem__", 2, key, value);
    if (result == NULL) {
        return -1;
    }
    return 0;
}

int
SbObject_DelItem(SbObject *o, SbObject *key)
{
    SbObject *result;

    result = SbObject_CallMethodObjArgs(o, "__delitem__", 1, key);
    if (result == NULL) {
        return -1;
    }
    return 0;
}


Sb_ssize_t
SbMapping_GetSize(SbObject *o)
{
    return SbObject_GetSize(o);
}

SbObject *
SbMapping_GetItem(SbObject *o, SbObject *key)
{
    return SbObject_GetItem(o, key);
}

SbObject *
SbMapping_GetItemString(SbObject *o, const char *key)
{
    SbObject *k;
    SbObject *result;

    k = SbStr_FromString(key);
    if (!k) {
        return NULL;
    }

    result = SbMapping_GetItem(o, k);
    Sb_DECREF(k);
    return result;
}

int
SbMapping_SetItem(SbObject *o, SbObject *key, SbObject *value)
{
    return SbObject_SetItem(o, key, value);
}

int
SbMapping_SetItemString(SbObject *o, const char *key, SbObject *value)
{
    SbObject *k;
    int result;

    k = SbStr_FromString(key);
    if (!k) {
        return -1;
    }

    result = SbMapping_SetItem(o, k, value);
    Sb_DECREF(k);
    return result;
}

int
SbMapping_DelItem(SbObject *o, SbObject *key)
{
    return SbObject_DelItem(o, key);
}

int
SbMapping_DelItemString(SbObject *o, const char *key)
{
    SbObject *k;
    int result;

    k = SbStr_FromString(key);
    if (!k) {
        return -1;
    }

    result = SbMapping_DelItem(o, k);
    Sb_DECREF(k);
    return result;
}


Sb_ssize_t
SbSequence_GetSize(SbObject *o)
{
    return SbMapping_GetSize(o);
}

SbObject *
SbSequence_GetItem(SbObject *o, Sb_ssize_t index)
{
    SbObject *key;
    SbObject *result;

    key = SbInt_FromNative(index);
    if (!key) {
        return NULL;
    }
    result = SbObject_GetItem(o, key);
    Sb_DECREF(key);
    return result;
}

int
SbSequence_SetItem(SbObject *o, Sb_ssize_t index, SbObject *value)
{
    SbObject *key;
    int result;

    key = SbInt_FromNative(index);
    if (!key) {
        return -1;
    }
    Sb_INCREF(value);
    result = SbObject_SetItem(o, key, value);
    Sb_DECREF(key);
    return result;
}

int
SbSequence_DelItem(SbObject *o, Sb_ssize_t index)
{
    SbObject *key;
    int result;

    key = SbInt_FromNative(index);
    if (!key) {
        return -1;
    }
    result = SbObject_DelItem(o, key);
    Sb_DECREF(key);
    return result;
}

SbObject *
SbSequence_GetSlice2(SbObject *o, Sb_ssize_t i1, Sb_ssize_t i2)
{
    return NULL;
}

int
SbSequence_SetSlice2(SbObject *o, Sb_ssize_t i1, Sb_ssize_t i2, SbObject *values)
{
    return -1;
}

int
SbSequence_DelSlice2(SbObject *o, Sb_ssize_t i1, Sb_ssize_t i2)
{
    return -1;
}
