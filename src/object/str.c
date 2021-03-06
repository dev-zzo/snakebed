#include "snakebed.h"
#include "internal.h"

/* Keep the type object here. */
SbTypeObject *SbStr_Type = NULL;

/*
 * C interface implementations
 */

SbObject *
SbStr_FromString(const char *v)
{
    Sb_ssize_t len;

    len = SbRT_StrLen(v);
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
            SbRT_MemCpy(op->items, v, len);
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
                    len = SbRT_StrLen(s);
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
        SbErr_RaiseWithString(SbExc_SystemError, "non-string object passed to a string method");
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
        SbErr_RaiseWithString(SbExc_SystemError, "non-string object passed to a string method");
        return NULL;
    }
#endif
    return SbStr_AsStringUnsafe(p);
}

int
SbStr_StartsWithString(SbObject *p1, const char *p2)
{
    Sb_ssize_t length;

    if (!SbStr_CheckExact(p1)) {
        return -1;
    }
    length = SbRT_StrLen(p2);
    if (length > SbStr_GetSizeUnsafe(p1)) {
        return 0;
    }
    return SbRT_MemCmp(SbStr_AsString(p1), p2, length) == 0;
}

SbObject *
SbStr_Join(SbObject *glue, SbObject *iterable)
{
    SbObject *it;
    SbObject *o;
    SbObject *result;
    Sb_ssize_t length;
    char *cursor;
    const char *source;

    it = SbObject_GetIter(iterable);
    if (!it) {
        return NULL;
    }

    length = -SbStr_GetSizeUnsafe(glue);
    for (o = SbIter_Next(it); o; o = SbIter_Next(it)) {
        if (!SbStr_CheckExact(o)) {
            Sb_DECREF(o);
            Sb_DECREF(it);
            SbErr_RaiseWithFormat(SbExc_TypeError, "invalid type: expected str, found %s", Sb_TYPE(o)->tp_name);
            return NULL;
        }
        length += SbStr_GetSizeUnsafe(o) + SbStr_GetSizeUnsafe(glue);
        Sb_DECREF(o);
    }
    Sb_DECREF(it);

    if (length <= 0) {
        return SbStr_FromString("");
    }

    /* FIXME: guard against overflows */
    result = SbStr_FromStringAndSize(NULL, length);
    if (!result) {
        return NULL;
    }

    it = SbObject_GetIter(iterable);
    if (!it) {
        Sb_DECREF(result);
        return NULL;
    }

    cursor = SbStr_AsStringUnsafe(result);

    o = SbIter_Next(it);
    goto midloop;

    while (o) {
        length = SbStr_GetSizeUnsafe(glue);
        source = SbStr_AsStringUnsafe(glue);
        while (length-- > 0) {
            *cursor++ = *source++;
        }
midloop:
        length = SbStr_GetSizeUnsafe(o);
        source = SbStr_AsStringUnsafe(o);
        while (length-- > 0) {
            *cursor++ = *source++;
        }
        Sb_DECREF(o);
        o = SbIter_Next(it);
    }
    Sb_DECREF(it);

    return result;
}

SbObject *
SbStr_Concat(SbObject *lhs, SbObject *rhs)
{
    SbObject *o_new;
    Sb_ssize_t lhs_size, rhs_size;
    char *new_buffer;

    /* OPT: Do not reallocate if one of strings is empty. */
    lhs_size = SbStr_GetSizeUnsafe(lhs);
    if (lhs_size == 0) {
        Sb_INCREF(rhs);
        return rhs;
    }
    rhs_size = SbStr_GetSizeUnsafe(rhs);
    if (rhs_size == 0) {
        Sb_INCREF(lhs);
        return lhs;
    }

    o_new = SbStr_FromStringAndSize(NULL, lhs_size + rhs_size);
    if (!o_new) {
        return NULL;
    }

    new_buffer = SbStr_AsStringUnsafe(o_new);
    SbRT_MemCpy(new_buffer, SbStr_AsStringUnsafe(lhs), lhs_size);
    SbRT_MemCpy(new_buffer + lhs_size, SbStr_AsStringUnsafe(rhs), rhs_size);

    return o_new;
}

int
SbStr_Truncate(SbObject *p, Sb_ssize_t new_length)
{
    SbStrObject *myself = (SbStrObject *)p;

    if (new_length > SbStr_GetSizeUnsafe(myself)) {
        return -1;
    }

    myself->ob_itemcount = new_length;
    SbStr_AsStringUnsafe(myself)[new_length] = '\0';
    return 0;
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
    return SbRT_MemCmp(SbStr_AsString(p1), SbStr_AsString(p2), length) == 0;
}

int
_SbStr_EqString(SbObject *p1, const char *p2)
{
    Sb_ssize_t length;

    if (!SbStr_CheckExact(p1)) {
        return -1;
    }
    length = SbRT_StrLen(p2);
    if (length != SbStr_GetSizeUnsafe(p1)) {
        return 0;
    }
    return SbRT_MemCmp(SbStr_AsString(p1), p2, length) == 0;
}


/* Python accessible methods */

static SbObject *
str_new(SbObject *dummy, SbObject *args, SbObject *kwargs)
{
    SbObject *o;

    if (SbArgs_Parse("O:cls|O:o", args, kwargs, &dummy, &o) < 0) {
        return NULL;
    }

    if (SbStr_CheckExact(o)) {
        Sb_INCREF(o);
        return o;
    }

    return SbObject_Str(o);
}

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

    if (SbArgs_Parse("O:index", args, kwargs, &index) < 0) {
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
        pos = SbInt_AsNative(index);
        if (pos == -1 && SbErr_Occurred()) {
            return NULL;
        }
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

static SbObject *
str_join(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *iterable;

    if (SbArgs_Parse("O:iterable", args, kwargs, &iterable) < 0) {
        return NULL;
    }
    return SbStr_Join(self, iterable);
}

static void
fill_ljust(char *dst, Sb_ssize_t width, const char *src, Sb_ssize_t src_width, char filler)
{
    SbRT_MemCpy(dst, src, src_width);
    SbRT_MemSet(dst + src_width, filler, width - src_width);
}

static void
fill_cjust(char *dst, Sb_ssize_t width, const char *src, Sb_ssize_t src_width, char filler)
{
    Sb_ssize_t a;

    a = (width - src_width) / 2;
    SbRT_MemSet(dst, filler, a);
    SbRT_MemCpy(dst + a, src, src_width);
    SbRT_MemSet(dst + a + src_width, filler, width - src_width - a);
}

static void
fill_rjust(char *dst, Sb_ssize_t width, const char *src, Sb_ssize_t src_width, char filler)
{
    SbRT_MemSet(dst, filler, width - src_width);
    SbRT_MemCpy(dst + width - src_width, src, src_width);
}

static SbObject *
str_justify_internal(SbObject *str, Sb_ssize_t width, char filler, void (*proc)(char *, Sb_ssize_t, const char *, Sb_ssize_t, char))
{
    SbObject *o_result;

    if (SbStr_GetSizeUnsafe(str) >= width) {
        Sb_INCREF(str);
        return str;
    }

    o_result = SbStr_FromStringAndSize(NULL, width);
    if (!o_result) {
        return NULL;
    }

    proc(SbStr_AsStringUnsafe(o_result), width, SbStr_AsStringUnsafe(str), SbStr_GetSizeUnsafe(str), filler);
    return o_result;
}

static SbObject *
str_justify_generic(SbObject *self, SbObject *args, SbObject *kwargs, void (*proc)(char *, Sb_ssize_t, const char *, Sb_ssize_t, char))
{
    char fillchar = ' ';
    Sb_ssize_t width;

    if (SbArgs_Parse("i:width|c:fillchar", args, kwargs, &width, &fillchar) < 0) {
        return NULL;
    }

    return str_justify_internal(self, width, fillchar, proc);
}

static SbObject *
str_ljust(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return str_justify_generic(self, args, kwargs, fill_ljust);
}

static SbObject *
str_center(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return str_justify_generic(self, args, kwargs, fill_cjust);
}

static SbObject *
str_rjust(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return str_justify_generic(self, args, kwargs, fill_rjust);
}

SbObject *
SbStr_JustifyLeft(SbObject *str, Sb_ssize_t width, char filler)
{
    return str_justify_internal(str, width, filler, fill_ljust);
}

SbObject *
SbStr_JustifyCenter(SbObject *str, Sb_ssize_t width, char filler)
{
    return str_justify_internal(str, width, filler, fill_cjust);
}

SbObject *
SbStr_JustifyRight(SbObject *str, Sb_ssize_t width, char filler)
{
    return str_justify_internal(str, width, filler, fill_rjust);
}

static int
str_normalize_indices(Sb_ssize_t *p_start, Sb_ssize_t *p_end, Sb_ssize_t len)
{
    Sb_ssize_t index;

    index = *p_start;
    if (index < 0) {
        index += len;
        if (index < 0) {
            goto index_oob;
        }
    }
    *p_start = index;

    index = *p_end;
    if (index < 0) {
        index += len;
        if (index < 0) {
            goto index_oob;
        }
    }
    if (index > len) {
        index = len;
    }
    *p_end = index;

    return 0;

index_oob:
    SbErr_RaiseWithString(SbExc_IndexError, "index out of range");
    return -1;
}

typedef Sb_ssize_t (*str_searcher_t)(const char *, Sb_ssize_t, const char *, Sb_ssize_t);

static Sb_ssize_t
str_find_internal(SbObject *self, SbObject *args, SbObject *kwargs, str_searcher_t finder)
{
    SbObject *o_pattern;
    Sb_ssize_t start;
    Sb_ssize_t end;
    Sb_ssize_t str_len;
    Sb_ssize_t pos;

    start = 0;
    end = SbStr_GetSizeUnsafe(self);

    if (SbArgs_Parse("S:pattern|i:start,i:end", args, kwargs, &o_pattern, &start, &end) < 0) {
        return -2;
    }

    str_len = SbStr_GetSizeUnsafe(self);
    if (str_normalize_indices(&start, &end, str_len) < 0) {
        return -2;
    }

    if (start <= str_len) {
        pos = finder(SbStr_AsStringUnsafe(self) + start, end - start,
            SbStr_AsStringUnsafe(o_pattern), SbStr_GetSizeUnsafe(o_pattern));
        if (pos >= 0) {
            return pos + start;
        }
    }

    return -1;
}

static SbObject *
str_xfind(SbObject *self, SbObject *args, SbObject *kwargs, str_searcher_t finder)
{
    Sb_ssize_t pos;

    pos = str_find_internal(self, args, kwargs, finder);
    if (pos >= -1) {
        return SbInt_FromNative(pos);
    }
    return NULL;
}

static SbObject *
str_find(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return str_xfind(self, args, kwargs, SbRT_MemMem);
}

static SbObject *
str_rfind(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return str_xfind(self, args, kwargs, SbRT_MemRMem);
}

static SbObject *
str_xindex(SbObject *self, SbObject *args, SbObject *kwargs, str_searcher_t finder)
{
    Sb_ssize_t pos;

    pos = str_find_internal(self, args, kwargs, finder);
    if (pos >= 0) {
        return SbInt_FromNative(pos);
    }
    if (pos == -1) {
        SbErr_RaiseWithString(SbExc_ValueError, "substring not found");
    }
    return NULL;
}

static SbObject *
str_index(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return str_xindex(self, args, kwargs, SbRT_MemMem);
}

static SbObject *
str_rindex(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return str_xindex(self, args, kwargs, SbRT_MemRMem);
}


static SbObject *
str_startswith(SbObject *self, SbObject *args, SbObject *kwargs)
{
    const char *prefix;

    /* NOTE: tuple prefix not implemented; start/end indices not implemented. */
    if (SbArgs_Parse("s:prefix", args, kwargs, &prefix) < 0) {
        return NULL;
    }
    switch (SbStr_StartsWithString(self, prefix)) {
    case 0:
        Sb_RETURN_FALSE;
    case 1:
        Sb_RETURN_TRUE;
    default:
        return NULL;
    }
}

static SbObject *
str_concat(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_rhs;

    if (SbArgs_Parse("O:rhs", args, kwargs, &o_rhs) < 0) {
        return NULL;
    }
    if (!SbStr_CheckExact(o_rhs)) {
        Sb_INCREF(Sb_NotImplemented);
        return Sb_NotImplemented;
    }
    return SbStr_Concat(self, o_rhs);
}

#if SUPPORTS(STR_FORMAT)

static SbObject *
str_format(SbObject *self, SbObject *args, SbObject *kwargs)
{
    const char *spec_string;
    SbString_FormatSpecifier spec;
    SbObject *o_result;

    if (SbArgs_Parse("z:spec", args, kwargs, &spec_string) < 0) {
        return NULL;
    }
    if (SbString_ParseFormatSpec(spec_string ? spec_string : "", &spec) < 0) {
        return NULL;
    }

    if (spec.precision >= 0 && (unsigned long)SbStr_GetSizeUnsafe(self) > spec.precision) {
        self = SbStr_FromStringAndSize(SbStr_AsStringUnsafe(self), spec.precision);
    }
    else {
        Sb_INCREF(self);
    }

    if (spec.align_flag == '>') {
        o_result = SbStr_JustifyRight(self, spec.min_width, spec.filler);
    }
    else if (spec.align_flag == '^') {
        o_result = SbStr_JustifyCenter(self, spec.min_width, spec.filler);
    }
    else {
        o_result = SbStr_JustifyLeft(self, spec.min_width, spec.filler);
    }
    Sb_DECREF(self);

    return o_result;
}

#endif /* SUPPORTS(STR_FORMAT) */

#if SUPPORTS(STRING_INTERPOLATION)

/* Ref: https://docs.python.org/2/library/stdtypes.html#string-formatting */

static SbObject *
str_interpolate(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbErr_RaiseWithString(SbExc_SystemError, "not implemented");
    return NULL;
}

#endif /* SUPPORTS(STRING_INTERPOLATION) */

SbObject *
Formatter_VFormat(SbObject *self, SbObject *args, SbObject *kwargs);

/* Type initializer */

static const SbCMethodDef str_methods[] = {
    { "__new__", str_new },
    { "__hash__", str_hash },
    { "__str__", str_str },
    { "__len__", str_len },
    { "__eq__", str_eq },
    { "__ne__", str_ne },
    { "__getitem__", str_getitem },
    { "__add__", str_concat },

    { "join", str_join },

    { "ljust", str_ljust },
    { "center", str_center },
    { "rjust", str_rjust },

    { "find", str_find },
    { "index", str_index },
    { "rfind", str_rfind },
    { "rindex", str_rindex },

    { "startswith", str_startswith },

#if SUPPORTS(STRING_INTERPOLATION)
    { "__mod__", str_interpolate },
#endif
#if SUPPORTS(STR_FORMAT)
    { "format", Formatter_VFormat },
    { "__format__", str_format },
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
