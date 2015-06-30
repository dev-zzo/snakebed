#include "snakebed.h"

int
SbArgs_ParseVa(SbObject *args, SbObject *kwds, Sb_ssize_t count_min, Sb_ssize_t count_max, const char *names[], va_list va)
{
    Sb_ssize_t passed_posarg_count;
    Sb_ssize_t arg_pos;

    passed_posarg_count = args ? SbTuple_GetSizeUnsafe(args) : 0;
    for (arg_pos = 0; arg_pos < count_max; ++arg_pos) {
        SbObject *arg_value;
        SbObject **arg_ptr;

        arg_value = NULL;
        if (arg_pos < passed_posarg_count) {
            arg_value = SbTuple_GetItemUnsafe(args, arg_pos);
            /* assert(arg_value); */
            Sb_INCREF(arg_value);
        }
        else {
            if (kwds) {
                const char  *arg_name;

                arg_name = names[arg_pos];
                arg_value = SbDict_GetItemString(kwds, arg_name);
                if (arg_value) {
                    Sb_INCREF(arg_value);
                    SbDict_DelItemString(kwds, arg_name);
                }
            }
        }

        if (!arg_value && arg_pos < count_min) {
            if (count_min == count_max) {
                SbErr_RaiseWithFormat(SbExc_TypeError, "function takes exactly %d args (%d passed)",
                    count_min,
                    passed_posarg_count);
            }
            else {
                SbErr_RaiseWithFormat(SbExc_TypeError, "function takes between %d and %d args (%d passed)",
                    count_min,
                    count_max,
                    passed_posarg_count);
            }
            goto fail0;
        }

        arg_ptr = va_arg(va, SbObject **);
        *arg_ptr = arg_value;
    }

    return 0;

fail0:
    return -1;
}

int
SbArgs_Parse(SbObject *args, SbObject *kwds, Sb_ssize_t count_min, Sb_ssize_t count_max, const char *names[], ...)
{
    int result;
    va_list va;

    va_start(va, names);
    result = SbArgs_ParseVa(args, kwds, count_min, count_max, names, va);
    va_end(va);
    return result;
}

int
SbArgs_Unpack(SbObject *args, Sb_ssize_t count_min, Sb_ssize_t count_max, ...)
{
    int result;
    va_list va;

    va_start(va, count_max);
    result = SbArgs_ParseVa(args, NULL, count_min, count_max, NULL, va);
    va_end(va);
    return result;
}

