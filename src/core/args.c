#include "snakebed.h"

int
SbArgs_ParseVa(SbObject *args, SbObject *kwds, Sb_ssize_t count_min, Sb_ssize_t count_max, const char *names[], ...)
{
    return -1;
}

int
SbArgs_Parse(SbObject *args, SbObject *kwds, Sb_ssize_t count_min, Sb_ssize_t count_max, const char *names[], ...)
{
    return -1;
}

int
SbArgs_Unpack(SbObject *args, Sb_ssize_t count_min, Sb_ssize_t count_max, ...)
{
    va_list va;
    Sb_ssize_t count, pos;

    if (!SbTuple_CheckExact(args)) {
        SbErr_RaiseWithFormat(SbErr_TypeError, "expected `%s` to be a tuple", "args");
        return -1;
    }

    count = SbTuple_GetSizeUnsafe(args);
    if (count < count_min || count > count_max) {
        if (count_min < count_max) {
            SbErr_RaiseWithFormat(SbErr_TypeError, "tuple may contain between %d and %d items (%d given)", count_min, count_max, count);
        }
        else {
            SbErr_RaiseWithFormat(SbErr_TypeError, "tuple may contain exactly %d items (%d given)", count_min, count);
        }
        return -1;
    }

    va_start(va, count_max);
    for (pos = 0; pos < count; ++pos) {
        SbObject **po;

        po = va_arg(va, SbObject **);
        if (!po) {
            SbErr_RaiseWithString(SbErr_ValueError, "a NULL pointer found while unpacking a tuple");
            return -1;
        }
        *po = SbTuple_GetItemUnsafe(args, pos);
    }
    va_end(va);

    return 0;
}

