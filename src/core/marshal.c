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

static long
read_byte(SbObject *input)
{
    Sb_byte_t buffer[1];

    if (SbFile_Read(input, buffer, sizeof(buffer)) < sizeof(buffer)) {
        /* Raise something */
        return -1;
    }
    return buffer[0];
}

static long
read_half(SbObject *input)
{
    Sb_byte_t buffer[2];

    if (SbFile_Read(input, buffer, sizeof(buffer)) < sizeof(buffer)) {
        /* Raise something */
        return -1;
    }
    return (buffer[1] << 8) | buffer[0];
}

static long
read_long(SbObject *input)
{
    Sb_byte_t buffer[4];

    if (SbFile_Read(input, buffer, sizeof(buffer)) < sizeof(buffer)) {
        /* Raise something */
        return -1;
    }
    return (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
}

static int
read_string(SbObject *input, void *buffer, Sb_size_t size)
{
    return SbFile_Read(input, buffer, size) == size;
}

static SbObject *
read_object(SbObject *input)
{
    char type_marker;
    long n;
    Sb_ssize_t pos;
    SbObject *result;

    if (SbFile_Read(input, &type_marker, 1) < 1) {
        SbErr_RaiseWithString(SbErr_ValueError, "marshal: premature EOF encountered");
        return NULL;
    }

    switch (type_marker) {
    case TYPE_NULL:
        result = NULL;
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
        result = SbInt_FromLong(read_long(input));
        break;
    case TYPE_STRING:
        n = read_long(input);
        result = SbStr_FromStringAndSize(NULL, n);
        if (!result) {
            break;
        }
        if (!read_string(input, SbStr_AsStringUnsafe(result), n)) {
            Sb_DECREF(result);
            result = NULL;
            break;
        }
        break;
    case TYPE_STRINGREF:
        n = read_long(input);
        break;
    case TYPE_TUPLE:
        n = read_long(input);
        result = SbTuple_New(n);
        if (!result) {
            break;
        }
        for (pos = 0; pos < n; ++pos) {
            SbObject *e;

            e = read_object(input);
            if (!e) {
                Sb_DECREF(result);
                result = NULL;
                break;
            }
            SbTuple_SetItemUnsafe(result, pos, e);
        }
        break;
    case TYPE_LIST:
        n = read_long(input);
        result = SbList_New(n);
        if (!result) {
            break;
        }
        for (pos = 0; pos < n; ++pos) {
            SbObject *e;

            e = read_object(input);
            if (!e) {
                Sb_DECREF(result);
                result = NULL;
                break;
            }
            SbList_SetItemUnsafe(result, pos, e);
        }
        break;
    case TYPE_DICT:
        result = SbDict_New();
        if (!result) {
            break;
        }
        for (;;) {
            SbObject *key;
            SbObject *value;

            key = read_object(input);
            if (!key) {
                if (SbErr_Occurred()) {
                    Sb_DECREF(result);
                    result = NULL;
                }
                break;
            }
            value = read_object(input);
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
    default:
        SbErr_RaiseWithString(SbErr_ValueError, "marshal: unknown data type");
        result = NULL;
        break;
    }

    return result;
}

SbObject *
Sb_ReadObject(const char *path)
{
    SbObject *input;
    SbObject *result;

    input = SbFile_New(path, "rb");
    if (!input) {
        return NULL;
    }

    result = read_object(input);
    SbFile_Close(input);
    return result;
}

