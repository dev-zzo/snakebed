#include "snakebed.h"

int
SbArgs_ParseVa(const char *spec, SbObject *args, SbObject *kwds, va_list va)
{
    char name_buffer[128];
    char *name_buffer_limit = &name_buffer[sizeof(name_buffer) / sizeof(name_buffer[0]) - 1];
    Sb_ssize_t arg_pos;
    Sb_ssize_t posarg_count;
    SbObject *arg;
    const char *expected_arg_type;
    int doing_optionals = 0;

    posarg_count = args ? SbTuple_GetSizeUnsafe(args) : 0;
    arg_pos = 0;
    while (*spec) {
        char conv;
        char *name_ptr;

        /* Check for the delimiters */
        if (*spec == '|') {
            doing_optionals = 1;
            ++spec;
        }
        if (*spec == ',') {
            ++spec;
        }

        /* Keep conv specifier */
        conv = *spec++;
        /* Skip colon */
        spec++;
        /* Copy the name */
        name_ptr = name_buffer;
        while (*spec && *spec != ',' && *spec != '|') {
            *name_ptr = *spec;
            spec++;
            if (name_ptr < name_buffer_limit) {
                name_ptr++;
            }
        }
        *name_ptr = '\0';

        /* Get the object */
        arg = NULL;
        if (arg_pos < posarg_count) {
            arg = SbTuple_GetItemUnsafe(args, arg_pos);
        }
        else if (kwds) {
            arg = SbDict_GetItemString(kwds, name_buffer);
        }

        if (arg) {
            switch (conv) {
            case 'S':
                if (!SbStr_CheckExact(arg)) {
                    expected_arg_type = "str";
                    goto invalid_arg_type;
                }
                goto store_ptr;

            case 'T':
                if (!SbTuple_CheckExact(arg)) {
                    expected_arg_type = "tuple";
                    goto invalid_arg_type;
                }
                goto store_ptr;

            case 'L':
                if (!SbList_CheckExact(arg)) {
                    expected_arg_type = "list";
                    goto invalid_arg_type;
                }
                goto store_ptr;

            case 'D':
                if (!SbDict_CheckExact(arg)) {
                    expected_arg_type = "dict";
                    goto invalid_arg_type;
                }
                goto store_ptr;

            case 'O':
store_ptr:
                *va_arg(va, SbObject **) = arg;
                break;

            case 'i':
                if (!SbInt_CheckExact(arg)) {
                    expected_arg_type = "int";
                    goto invalid_arg_type;
                }
                *va_arg(va, SbInt_Native_t *) = SbInt_AsNative(arg);
                if (SbErr_Occurred()) {
                    return -1;
                }
                break;

            case 'z':
                if (arg == Sb_None) {
                    *va_arg(va, const char **) = NULL;
                    break;
                }
                /* Fall through */
            case 's':
                if (!SbStr_CheckExact(arg)) {
                    expected_arg_type = "str";
                    goto invalid_arg_type;
                }
                *va_arg(va, const char **) = (const char *)SbStr_AsStringUnsafe(arg);
                break;

            case 'c':
                if (!SbStr_CheckExact(arg)) {
                    expected_arg_type = "str";
                    goto invalid_arg_type;
                }
                if (SbStr_GetSizeUnsafe(arg) != 1) {
                    SbErr_RaiseWithFormat(SbExc_ValueError, "expected arg '%s' to be of length 1, got %d",
                        name_buffer, SbStr_GetSizeUnsafe(arg));
                    return -1;
                }
                *va_arg(va, char *) = SbStr_AsStringUnsafe(arg)[0];
                break;

            default:
                SbErr_RaiseWithFormat(SbExc_ValueError, "unexpected conversion: %c", conv);
                return -1;
            }
        }
        else {
            if (!doing_optionals) {
                SbErr_RaiseWithFormat(SbExc_TypeError, "argument '%s' is required", name_buffer);
                return -1;
            }
        }

        ++arg_pos;
    }

    return 0;

invalid_arg_type:
    SbErr_RaiseWithFormat(SbExc_TypeError, "expected arg '%s' to be %s, got %s",
        name_buffer, expected_arg_type, Sb_TYPE(arg)->tp_name);
    return -1;
}

int
SbArgs_Parse(const char *spec, SbObject *args, SbObject *kwds, ...)
{
    int result;
    va_list va;

    va_start(va, kwds);
    result = SbArgs_ParseVa(spec, args, kwds, va);
    va_end(va);
    return result;
}

int
SbArgs_NoArgs(SbObject *args, SbObject *kwds)
{
    return 0;
}
