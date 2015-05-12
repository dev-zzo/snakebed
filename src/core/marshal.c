#include "snakebed.h"

#define TYPE_NULL               '0'

#define TYPE_NONE               'N'
#define TYPE_FALSE              'F'
#define TYPE_TRUE               'T'
#define TYPE_ELLIPSIS           '.'

#define TYPE_INT                'i'
#define TYPE_STRING8            's'
#define TYPE_STRING32           'S'
#define TYPE_STRINGREF8         'r'
#define TYPE_STRINGREF16        'R'
#define TYPE_TUPLE              '('
#define TYPE_LIST               '['
#define TYPE_DICT               '{'
#define TYPE_CODE               'c'

typedef struct _marshal_state {
    SbObject *strtab;
} marshal_state;

static int
read_byte(SbObject *input, long *value)
{
    Sb_byte_t buffer[1];

    if (SbFile_Read(input, buffer, sizeof(buffer)) < sizeof(buffer)) {
        /* Raise something */
        return -1;
    }
    *value = buffer[0];
    return 0;
}

static int
read_half(SbObject *input, long *value)
{
    Sb_byte_t buffer[2];

    if (SbFile_Read(input, buffer, sizeof(buffer)) < sizeof(buffer)) {
        /* Raise something */
        return -1;
    }
    *value =  (buffer[1] << 8) | buffer[0];
    return 0;
}

static int
read_int(SbObject *input, SbInt_Native_t *value)
{
    Sb_byte_t buffer[4];

    if (SbFile_Read(input, buffer, sizeof(buffer)) < sizeof(buffer)) {
        /* Raise something */
        return -1;
    }
    *value =  (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
    return 0;
}

static int
read_string(SbObject *input, void *buffer, Sb_size_t size)
{
    return SbFile_Read(input, buffer, size) == size ? 0 : -1;
}

static SbObject *
read_object(SbObject *input, marshal_state *state)
{
    char type_marker;
    SbInt_Native_t n;
    Sb_ssize_t pos;
    SbObject *result = NULL;

    if (SbFile_Read(input, &type_marker, 1) < 1) {
        SbErr_RaiseWithString(SbErr_ValueError, "marshal: premature EOF encountered");
        return NULL;
    }

    switch (type_marker) {
    case TYPE_NULL:
        break;

    case TYPE_NONE:
        Sb_INCREF(Sb_None);
        result = Sb_None;
        break;

    case TYPE_TRUE:
        Sb_INCREF(Sb_True);
        result = Sb_True;
        break;

    case TYPE_FALSE:
        Sb_INCREF(Sb_False);
        result = Sb_False;
        break;

    case TYPE_INT:
        if (read_int(input, &n) < 0) {
            break;
        }
        result = SbInt_FromNative(n);
        break;

    case TYPE_STRING8:
        if (read_byte(input, &n) < 0) {
            break;
        }
        goto do_string;
    case TYPE_STRING32:
        if (read_int(input, &n) < 0) {
            break;
        }
do_string:
        result = SbStr_FromStringAndSize(NULL, n);
        if (!result) {
            break;
        }
        if (read_string(input, SbStr_AsStringUnsafe(result), n) < 0) {
            Sb_DECREF(result);
            result = NULL;
            break;
        }
        SbList_Append(state->strtab, result);
        break;

    case TYPE_STRINGREF8:
        if (read_byte(input, &n) < 0) {
            break;
        }
        goto do_strref;
    case TYPE_STRINGREF16:
        if (read_half(input, &n) < 0) {
            break;
        }
do_strref:
        if (n >= SbList_GetSizeUnsafe(state->strtab)) {
            SbErr_RaiseWithString(SbErr_ValueError, "marshal: strref out of bounds");
            result = NULL;
            break;
        }
        result = SbList_GetItemUnsafe(state->strtab, n);
        Sb_INCREF(result);
        break;

    case TYPE_TUPLE:
        if (read_int(input, &n) < 0) {
            break;
        }
        result = SbTuple_New(n);
        if (!result) {
            break;
        }
        for (pos = 0; pos < n; ++pos) {
            SbObject *e;

            e = read_object(input, state);
            if (!e) {
                Sb_DECREF(result);
                result = NULL;
                break;
            }
            SbTuple_SetItemUnsafe(result, pos, e);
        }
        break;

#if SUPPORTS_UNMARSHAL_LIST
    case TYPE_LIST:
        if (read_int(input, &n) < 0) {
            break;
        }
        result = SbList_New(n);
        if (!result) {
            break;
        }
        for (pos = 0; pos < n; ++pos) {
            SbObject *e;

            e = read_object(input, state);
            if (!e) {
                Sb_DECREF(result);
                result = NULL;
                break;
            }
            SbList_SetItemUnsafe(result, pos, e);
        }
        break;
#endif /* SUPPORTS_UNMARSHAL_LIST */

#if SUPPORTS_UNMARSHAL_DICT
    case TYPE_DICT:
        result = SbDict_New();
        if (!result) {
            break;
        }
        for (;;) {
            SbObject *key;
            SbObject *value;

            key = read_object(input, state);
            if (!key) {
                if (SbErr_Occurred()) {
                    Sb_DECREF(result);
                    result = NULL;
                }
                break;
            }
            value = read_object(input, state);
            if (!value) {
                Sb_DECREF(key);
                Sb_DECREF(result);
                result = NULL;
                break;
            }
            SbDict_SetItem(result, key, value);
            Sb_DECREF(key);
            Sb_DECREF(value);
        }
        break;
#endif /* SUPPORTS_UNMARSHAL_DICT */

    case TYPE_CODE:
        {
            SbObject *name;
            long flags;
            long stack_size;
            long arg_count;
            SbObject *code;
            SbObject *consts;
            SbObject *names;
            SbObject *varnames;

            name = read_object(input, state);
            if (!name) {
                break;
            }
            if (!SbStr_CheckExact(name)) {
                goto code_end_1;
            }
            if (read_int(input, &flags) < 0) {
                goto code_end_1;
            }
            if (read_int(input, &stack_size) < 0) {
                goto code_end_1;
            }
            if (read_int(input, &arg_count) < 0) {
                goto code_end_1;
            }
            code = read_object(input, state);
            if (!code) {
                goto code_end_1;
            }
            if (!SbStr_CheckExact(code)) {
                goto code_end_2;
            }
            consts = read_object(input, state);
            if (!consts) {
                goto code_end_2;
            }
            if (!SbTuple_CheckExact(consts)) {
                goto code_end_3;
            }
            names = read_object(input, state);
            if (!names) {
                goto code_end_3;
            }
            if (!SbTuple_CheckExact(names)) {
                goto code_end_4;
            }
            varnames = read_object(input, state);
            if (!varnames) {
                goto code_end_4;
            }
            if (!SbTuple_CheckExact(varnames)) {
                goto code_end_5;
            }

            result = SbCode_New(name, flags, stack_size, arg_count, code, consts, names, varnames);

code_end_5:
            Sb_DECREF(varnames);
code_end_4:
            Sb_DECREF(names);
code_end_3:
            Sb_DECREF(consts);
code_end_2:
            Sb_DECREF(code);
code_end_1:
            Sb_DECREF(name);
            break;
        }

    default:
        SbErr_RaiseWithString(SbErr_ValueError, "marshal: unknown data type");
        result = NULL;
        break;
    }

    return result;
}

SbObject *
Sb_ReadObjectFromPath(const char *path)
{
    SbObject *input;
    SbObject *result = NULL;
    marshal_state state;
    char signature[16];

    input = SbFile_New(path, "rb");
    if (!input) {
        return NULL;
    }

    state.strtab = SbList_New(0);
    if (!state.strtab) {
        goto exit1;
    }

    if (read_string(input, signature, 16) < 0) {
        goto exit1;
    }

    result = read_object(input, &state);
    Sb_DECREF(state.strtab);

exit1:
    Sb_DECREF(input);
    return result;
}

