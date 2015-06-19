#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbStr_Type = NULL;

/*
 * C interface implementations
 */

SbObject *
SbStr_FromString(const char *v)
{
    Sb_ssize_t len;

    len = Sb_StrLen(v);
    return SbStr_FromStringAndSize(v, len);
}

SbObject *
SbStr_FromStringAndSize(const void *v, Sb_ssize_t len)
{
    SbStrObject *op;

    op = (SbStrObject *)SbObject_NewVar(SbStr_Type, len);
    if (op) {
        op->stored_hash = -1;
        if (v) {
            Sb_MemCpy(op->items, v, len);
            op->items[len] = '\0';
        }
    }
    return (SbObject *)op;
}

Sb_ssize_t
str_format_internal_va(char *buffer, const char *format, va_list va)
{
    const char *p;
    int got_format;
    char *cursor;
    char temp_buf[32];

    got_format = 0;
    cursor = buffer;
    for (p = format; *p; ++p) {
        if (got_format) {
            const char *s;

            got_format = 0;
            switch (*p) {
            case '%':
                s = "%";
                break;

            case 'c':
                temp_buf[0] = (char)va_arg(va, int);
                temp_buf[1] = '\0';
                s = temp_buf;
                break;

            case 's':
                s = va_arg(va, const char *);
                break;

            case 'd':
            case 'i':
                s = Sb_LtoA(va_arg(va, int), 10);
                break;

            case 'u':
                s = Sb_ULtoA(va_arg(va, unsigned int), 10);
                break;

            case 'p':
                {
                    Sb_size_t len;

                    /* This is the only one that needs alignment... */
                    s = Sb_ULtoA((unsigned long)va_arg(va, void *), 16);
                    if (buffer) {
                        cursor[0] = '0';
                        cursor[1] = 'x';
                    }
                    cursor += 2;
                    len = Sb_StrLen(s);
                    if (buffer) {
                        while (len < 8) {
                            *cursor++ = '0';
                            ++len;
                        }
                    }
                    else {
                        if (len < 8) {
                            cursor += 8 - len;
                        }
                    }
                }
                break;

            case 'x':
                s = Sb_ULtoA(va_arg(va, unsigned int), 16);
                break;

            case 'l':
            case 'z':
            default:
                /* Invalid format specifier */
                /* NOTE: This differs from CPython. */
                continue;
            }

            if (buffer) {
                while (*cursor = *s++) {
                    ++cursor;
                }
            }
            else {
                while (*s++) {
                    ++cursor;
                }
            }
            continue;
        }

        if (*p == '%') {
            got_format = 1;
            continue;
        }

        if (buffer) {
            *cursor = *p;
        }
        cursor++;
    }
    if (buffer) {
        *cursor = '\0';
    }
    cursor++;

    return cursor - buffer;
}
SbObject *
SbStr_FromFormatVa(const char *format, va_list va)
{
    Sb_ssize_t max_length;
    SbObject *result;
    char *buffer;

    /* Determine maximum required size */
    max_length = str_format_internal_va(NULL, format, va);

    /* Allocate */
    result = SbStr_FromStringAndSize(NULL, max_length);
    if (!result) {
        return result;
    }
    buffer = SbStr_AsStringUnsafe(result);

    /* Write out */
    str_format_internal_va(buffer, format, va);

    return result;
}

SbObject *
SbStr_FromFormat(const char *format, ...)
{
    va_list va;
    SbObject *result;

    va_start(va, format);
    result = SbStr_FromFormatVa(format, va);
    va_end(va);

    return result;
}

Sb_ssize_t
SbStr_GetSize(SbObject *p)
{
#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbStr_CheckExact(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-string object passed to a string method");
        return -1;
    }
#endif

    return SbStr_GetSizeUnsafe(p);
}

const Sb_byte_t *
SbStr_AsString(SbObject *p)
{
#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbStr_CheckExact(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-string object passed to a string method");
        return NULL;
    }
#endif
    return SbStr_AsStringUnsafe(p);
}

long
_SbStr_Hash(SbObject *p)
{
    SbStrObject *op = (SbStrObject *)p;
    long x;

    if (op->stored_hash != -1)
        return op->stored_hash;

    x = _SbStr_HashString(SbStr_AsStringUnsafe(p), SbStr_GetSizeUnsafe(p));
    op->stored_hash = x;
    return x;
}

long
_SbStr_HashString(const Sb_byte_t *p, Sb_ssize_t len)
{
    Sb_ssize_t count = len;
    long x;

    x = *p << 7;
    while (count-- > 0) {
        x = (1000003 * x) ^ *p;
        p++;
    }
    x ^= len;
    if (x == -1) {
        x = -2;
    }
    return x;
}

int
_SbStr_Eq(SbObject *p1, SbObject *p2)
{
    Sb_ssize_t length;

    if (!SbStr_CheckExact(p1) || !SbStr_CheckExact(p2)) {
        return -1;
    }
    length = SbStr_GetSizeUnsafe(p2);
    if (length != SbStr_GetSizeUnsafe(p1)) {
        return 0;
    }
    return Sb_MemCmp(SbStr_AsString(p1), SbStr_AsString(p2), length) == 0;
}

int
_SbStr_EqString(SbObject *p1, const char *p2)
{
    Sb_ssize_t length;

    if (!SbStr_CheckExact(p1)) {
        return -1;
    }
    length = Sb_StrLen(p2);
    if (length != SbStr_GetSizeUnsafe(p1)) {
        return 0;
    }
    return Sb_MemCmp(SbStr_AsString(p1), p2, length) == 0;
}


/* Python accessible methods */

static SbObject *
str_hash(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbInt_FromNative(_SbStr_Hash(self));
}

static SbObject *
str_str(SbObject *self, SbObject *args, SbObject *kwargs)
{
    Sb_INCREF(self);
    return self;
}

static SbObject *
str_len(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbInt_FromNative(SbStr_GetSizeUnsafe(self));
}

static SbObject *
str_eq(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *other;

    other = SbTuple_GetItem(args, 0);
    if (other) {
        return SbBool_FromLong(_SbStr_Eq(self, other));
    }
    return NULL;
}

static SbObject *
str_ne(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *other;

    other = SbTuple_GetItem(args, 0);
    if (other) {
        return SbBool_FromLong(!_SbStr_Eq(self, other));
    }
    return NULL;
}

static SbObject *
str_getitem(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *index;
    SbObject *result;
    SbInt_Native_t pos;

    if (SbArgs_Unpack(args, 1, 1, &index) < 0) {
        return NULL;
    }
    if (SbSlice_Check(index)) {
        Sb_ssize_t start, end, step, slice_length;
        char *src_buffer;
        char *dst_buffer;

        if (SbSlice_GetIndices(index, SbList_GetSizeUnsafe(self), &start, &end, &step, &slice_length) < 0) {
            return NULL;
        }

        src_buffer = (char *)SbStr_AsStringUnsafe(self);
        result = SbStr_FromStringAndSize(NULL, slice_length);
        dst_buffer = (char *)SbStr_AsStringUnsafe(result);
        pos = 0;
        for ( ; start < end; start += step) {
            dst_buffer[pos++] = src_buffer[start];
        }
        /* SAFE: we overallocate by 1 */
        dst_buffer[pos] = '\0';

        return result;
    }
    if (SbInt_Check(index)) {
        pos = SbInt_AsNativeUnsafe(index);
        result = SbStr_FromStringAndSize(NULL, 1);
        if (result) {
            char *dst_buffer;

            dst_buffer = (char *)SbStr_AsStringUnsafe(result);
            dst_buffer[0] = SbStr_AsStringUnsafe(self)[pos];
            /* SAFE: we overallocate by 1 */
            dst_buffer[1] = '\0';
        }
        return result;
    }
    return _SbErr_IncorrectSubscriptType(index);
}

#if SUPPORTS(STR_FORMAT)

/* Ref: https://www.python.org/dev/peps/pep-3101/ */

struct str_conv_specifier {
    char filler;
    char align_flag;
    char sign_flag;
    char use_alt_form;
    char use_precision;
    char conv_type;
    unsigned long min_width;
    unsigned long precision;
};

int
_SbStr_ParseFormatSpec(const char *spec, struct str_conv_specifier* result)
{
    result->min_width = 0;
    result->precision = -1;
    result->conv_type = '\0';

    if (spec[0] != '\0' && (spec[1] == '<' || spec[1] == '>' || spec[1] == '=' || spec[1] == '^')) {
        result->filler = spec[0];
        result->align_flag = spec[1];
    }
    else {
        result->filler = ' ';
        if (*spec == '<' || *spec == '>' || *spec == '=' || *spec == '^') {
            result->align_flag = *spec;
            ++spec;
        }
        else {
            result->align_flag = '<';
        }
    }
    if (*spec == '+' || *spec == '-' || *spec == ' ') {
        result->sign_flag = *spec;
        ++spec;
    }
    if (*spec == '#') {
        result->use_alt_form = *spec;
        ++spec;
    }
    if (*spec == '0') {
        result->filler = '0';
        result->align_flag = '=';
        ++spec;
    }
    if (Sb_AtoUL(spec, &spec, 10, &result->min_width) < 0) {
        result->min_width = 0;
    }
    result->use_precision = 0;
    if (*spec == '.') {
        ++spec;
        if (Sb_AtoUL(spec, &spec, 10, &result->precision) >= 0) {
            result->use_precision = 1;
        }
    }
    if (*spec != '\0') {
        result->conv_type = *spec;
    }

    return 0;
}

static SbObject *
str_format(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbErr_RaiseWithString(SbErr_SystemError, "not implemented");
    return NULL;
}

#endif /* SUPPORTS(STR_FORMAT) */

#if SUPPORTS(STRING_INTERPOLATION)

/* Ref: https://docs.python.org/2/library/stdtypes.html#string-formatting */

static SbObject *
str_interpolate(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbErr_RaiseWithString(SbErr_SystemError, "not implemented");
    return NULL;
}

#endif /* SUPPORTS(STRING_INTERPOLATION) */

/* Type initializer */

static const SbCMethodDef str_methods[] = {
    { "__hash__", str_hash },
    { "__str__", str_str },
    { "__len__", str_len },
    { "__eq__", str_eq },
    { "__ne__", str_ne },
    { "__getitem__", str_getitem },
#if SUPPORTS(STRING_INTERPOLATION)
    { "__mod__", str_interpolate },
#endif
#if SUPPORTS(STR_FORMAT)
    { "format", str_format },
#endif
    /* Sentinel */
    { NULL, NULL },
};

int
_SbStr_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("str", NULL, NULL);
    if (!tp) {
        return -1;
    }

    /* This overallocates by 1 char -- is used for NUL terminator. */
    tp->tp_basicsize = sizeof(SbStrObject);
    tp->tp_itemsize = sizeof(char);
    tp->tp_destroy = SbObject_DefaultDestroy;

    SbStr_Type = tp;
    return 0;
}

int
_SbStr_BuiltinInit2()
{
    SbObject *dict;

    dict = _SbType_BuildMethodDict(str_methods);
    SbStr_Type->tp_dict = dict;
    return dict ? 0 : -1;
}
