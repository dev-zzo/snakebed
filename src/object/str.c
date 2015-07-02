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
    length = Sb_StrLen(p2);
    if (length > SbStr_GetSizeUnsafe(p1)) {
        return 0;
    }
    return Sb_MemCmp(SbStr_AsString(p1), p2, length) == 0;
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

static SbObject *
str_join(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *iterable;
    SbObject *it;
    SbObject *o;
    SbObject *result;
    Sb_ssize_t total_length;
    Sb_ssize_t glue_count;
    char *cursor;

    if (SbArgs_Unpack(args, 1, 1, &iterable) < 0) {
        return NULL;
    }

    it = SbObject_GetIter(iterable);
    if (!it) {
        return NULL;
    }

    total_length = 0;
    glue_count = -1;
    for (o = SbIter_Next(it); o; o = SbIter_Next(it)) {
        if (!SbStr_CheckExact(o)) {
            Sb_DECREF(o);
            Sb_DECREF(it);
            SbErr_RaiseWithFormat(SbExc_TypeError, "invalid type: expected str, found %s", Sb_TYPE(o)->tp_name);
            return NULL;
        }
        total_length += SbStr_GetSizeUnsafe(o);
        glue_count += 1;
        Sb_DECREF(o);
    }
    Sb_DECREF(it);

    result = SbStr_FromStringAndSize(NULL, total_length + glue_count * SbStr_GetSizeUnsafe(self));
    if (!result) {
        return NULL;
    }

    it = SbObject_GetIter(iterable);
    if (!it) {
        return NULL;
    }

    cursor = SbStr_AsStringUnsafe(result);
    for (o = SbIter_Next(it); o; o = SbIter_Next(it)) {
        Sb_MemCpy(cursor, SbStr_AsStringUnsafe(o), SbStr_GetSizeUnsafe(o));
        cursor += SbStr_GetSizeUnsafe(o);
        Sb_DECREF(o);
        if (glue_count > 0) {
            Sb_MemCpy(cursor, SbStr_AsStringUnsafe(self), SbStr_GetSizeUnsafe(self));
            cursor += SbStr_GetSizeUnsafe(self);
            glue_count -= 1;
        }
    }
    Sb_DECREF(it);

    return result;
}

static void
fill_ljust(char *dst, Sb_ssize_t width, const char *src, Sb_ssize_t src_width, char filler)
{
    Sb_MemCpy(dst, src, src_width);
    Sb_MemSet(dst + src_width, filler, width - src_width);
}

static void
fill_cjust(char *dst, Sb_ssize_t width, const char *src, Sb_ssize_t src_width, char filler)
{
    Sb_ssize_t a;

    a = (width - src_width) / 2;
    Sb_MemSet(dst, filler, a);
    Sb_MemCpy(dst + a, src, src_width);
    Sb_MemSet(dst + a + src_width, filler, width - src_width - a);
}

static void
fill_rjust(char *dst, Sb_ssize_t width, const char *src, Sb_ssize_t src_width, char filler)
{
    Sb_MemSet(dst, filler, width - src_width);
    Sb_MemCpy(dst + width - src_width, src, src_width);
}

static SbObject *
str_justify_generic(SbObject *self, SbObject *args, SbObject *kwargs, void (*proc)(char *, Sb_ssize_t, const char *, Sb_ssize_t, char))
{
    SbObject *o_width;
    SbObject *o_fillchar = NULL;
    char fillchar = ' ';
    Sb_ssize_t width;
    SbObject *o_result;

    if (SbArgs_Unpack(args, 1, 2, &o_width, &o_fillchar) < 0) {
        return NULL;
    }
    if (o_fillchar) {
        if (!SbStr_CheckExact(o_fillchar) || SbStr_GetSizeUnsafe(o_fillchar) != 1) {
            SbErr_RaiseWithString(SbExc_TypeError, "expected str of length 1 as fillchar");
            return NULL;
        }
        fillchar = SbStr_AsStringUnsafe(o_fillchar)[0];
    }
    if (SbInt_Check(o_width)) {
        width = SbInt_AsNative(o_width);
    }
    else {
        SbErr_RaiseWithString(SbExc_TypeError, "expected int as width");
        return NULL;
    }

    if (SbStr_GetSizeUnsafe(self) >= width) {
        Sb_INCREF(self);
        return self;
    }

    o_result = SbStr_FromStringAndSize(NULL, width);
    if (!o_result) {
        return NULL;
    }

    proc(SbStr_AsStringUnsafe(o_result), width, SbStr_AsStringUnsafe(self), SbStr_GetSizeUnsafe(self), fillchar);
    return o_result;
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
    SbObject *o_start = NULL;
    SbObject *o_end = NULL;
    Sb_ssize_t start;
    Sb_ssize_t end;
    Sb_ssize_t str_len;
    Sb_ssize_t pos;

    if (SbArgs_Unpack(args, 1, 3, &o_pattern, &o_start, &o_end) < 0) {
        return -2;
    }
    if (!SbStr_CheckExact(o_pattern)) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected str as pattern");
        return -2;
    }

    start = 0;
    end = SbStr_GetSizeUnsafe(self);
    if (o_start) {
        if (!SbInt_Check(o_start)) {
            SbErr_RaiseWithString(SbExc_TypeError, "expected int as start");
            return -2;
        }
        start = SbInt_AsNative(o_start);
    }
    if (o_end) {
        if (!SbInt_Check(o_end)) {
            SbErr_RaiseWithString(SbExc_TypeError, "expected int as end");
            return -2;
        }
        end = SbInt_AsNative(o_end);
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
    return str_xfind(self, args, kwargs, Sb_MemMem);
}

static SbObject *
str_rfind(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return str_xfind(self, args, kwargs, Sb_MemRMem);
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
    return str_xindex(self, args, kwargs, Sb_MemMem);
}

static SbObject *
str_rindex(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return str_xindex(self, args, kwargs, Sb_MemRMem);
}


static SbObject *
str_startswith(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_prefix;
    const char *prefix;

    /* NOTE: tuple prefix not implemented; start/end indices not implemented. */
    if (SbArgs_Unpack(args, 1, 1, &o_prefix) < 0) {
        return NULL;
    }
    if (!SbStr_CheckExact(o_prefix)) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected str as prefix");
        return NULL;
    }
    prefix = SbStr_AsStringUnsafe(o_prefix);
    switch (SbStr_StartsWithString(self, prefix)) {
    case 0:
        Sb_RETURN_FALSE;
    case 1:
        Sb_RETURN_TRUE;
    default:
        return NULL;
    }
}


#if SUPPORTS(STR_FORMAT)

/* Ref: https://www.python.org/dev/peps/pep-3101/ */

/*
replacement_field ::=  "{" [field_name] ["!" conversion] [":" format_spec] "}"
field_name        ::=  arg_name ("." attribute_name | "[" element_index "]")*
arg_name          ::=  [identifier | integer]
attribute_name    ::=  identifier
element_index     ::=  integer | index_string
index_string      ::=  <any source character except "]"> +
conversion        ::=  "r" | "s"
format_spec       ::=  <described in the next section>
*/

static SbObject *
formatter_parse(SbObject *format_string)
{
    const char *cursor;
    const char *cursor_limit;
    SbObject *items;
    SbObject *item;

    items = SbList_New(0);
    if (!items) {
        return NULL;
    }

    /* Formatter.parse() implementation. */
    cursor = SbStr_AsStringUnsafe(format_string);
    cursor_limit = cursor + SbStr_GetSizeUnsafe(format_string);
    while (cursor < cursor_limit) {
        const char *literal_start;
        Sb_ssize_t literal_length;

        item = SbTuple_New(4);
        if (!item) {
            goto error_exit;
        }

        literal_start = cursor;
        while (cursor < cursor_limit) {
            if (cursor[0] == '{' && cursor[1] != '{') {
                break;
            }
            ++cursor;
        }
        literal_length = cursor - literal_start;
        SbTuple_SetItemUnsafe(item, 0, SbStr_FromStringAndSize(literal_start, literal_length));

        if (cursor[0] == '{') {
            const char *start;

            /* NOTE: Expect } to close the replacement field. */
            ++cursor;

            /* Name */
            start = cursor;
            while (cursor[0] != ':' && cursor[0] != '!' && cursor[0] != '}') {
                if (cursor >= cursor_limit) {
                    Sb_DECREF(item);
                    goto premature_eol;
                }
                ++cursor;
            }
            SbTuple_SetItemUnsafe(item, 1, SbStr_FromStringAndSize(start, cursor - start));

            /* Conversion specifier */
            if (cursor[0] == '!') {
                start = cursor + 1;
                cursor += 2;
                if (cursor >= cursor_limit) {
                    Sb_DECREF(item);
                    goto premature_eol;
                }
                /* Make sure the format is correct. */
                if (cursor[0] != ':' && cursor[0] != '}') {
                    Sb_DECREF(item);
                    goto incorrect_format;
                }
                SbTuple_SetItemUnsafe(item, 3, SbStr_FromStringAndSize(start, 1));
            }
            else {
                SbTuple_SetNoneUnsafe(item, 3);
            }

            /* Formatting specifier */
            if (cursor[0] == ':') {
                ++cursor;
                start = cursor;
                while (cursor[0] != '}') {
                    if (cursor >= cursor_limit) {
                        Sb_DECREF(item);
                        goto unmatched_brace;
                    }
                    ++cursor;
                }
                SbTuple_SetItemUnsafe(item, 2, SbStr_FromStringAndSize(start, cursor - start));
            }
            else {
                SbTuple_SetNoneUnsafe(item, 2);
            }

            if (cursor[0] != '}') {
                Sb_DECREF(item);
                goto unmatched_brace;
            }
        }
        else {
            SbTuple_SetNoneUnsafe(item, 1);
            SbTuple_SetNoneUnsafe(item, 2);
            SbTuple_SetNoneUnsafe(item, 3);
        }
        SbList_Append(items, item);
    }

    return items;

premature_eol:
unmatched_brace:
    SbErr_RaiseWithString(SbExc_ValueError, "incorrect format string");

incorrect_format:
    SbErr_RaiseWithString(SbExc_ValueError, "incorrect format string");

error_exit:
    Sb_DECREF(items);
    return NULL;
}

static SbObject *
formatter_get_value(SbObject *name, SbObject *args, SbObject *kwargs)
{
    const char *str_name;
    SbObject *o;

    str_name = SbStr_AsStringUnsafe(name);
    if (Sb_IsDigit(str_name[0])) {
        unsigned long index;
        const char *pend;

        if (Sb_AtoUL(str_name, &pend, 10, &index) >= 0 && pend == str_name + SbStr_GetSizeUnsafe(name)) {
            o = SbTuple_GetItem(args, index);
            return o;
        }
    }
    o = SbDict_GetItem(kwargs, name);
    if (!o) {
        SbErr_RaiseWithObject(SbExc_KeyError, name);
    }
    return o;
}

static SbObject *
formatter_get_field(SbObject *field_name, SbObject *args, SbObject *kwargs)
{
    const char *start, *cursor;
    SbObject *o;
    SbObject *name;

    cursor = SbStr_AsStringUnsafe(field_name);

    /* Parse the first name component */
    start = cursor;
    while (cursor[0] && cursor[0] != '[' && cursor[0] != '.') {
        ++cursor;
    }

    name = SbStr_FromStringAndSize(start, cursor - start);
    o = formatter_get_value(name, args, kwargs);
    Sb_DECREF(name);
    if (!o) {
        return NULL;
    }

    while (cursor[0]) {
        char op = cursor[0];

        ++cursor;
        start = cursor;
        if (op == '[') {
            while (cursor[0] && cursor[0] != ']') {
                ++cursor;
            }
            if (cursor == start) {
                goto empty_attr;
            }
            /* TODO: index. */
        }
        else if (op == '.') {
            while (cursor[0] && cursor[0] != '[' && cursor[0] != '.') {
                ++cursor;
            }
            if (cursor == start) {
                goto empty_attr;
            }
            /* TODO: getattr. */
        }
        else {
            SbErr_RaiseWithString(SbExc_ValueError, "only '[' or '.' may follow ']' in field name");
            return NULL;
        }
    }

    return o;

empty_attr:
    SbErr_RaiseWithString(SbExc_ValueError, "empty attribute in field name");
    return NULL;
}

static SbObject *
str_format(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *items;
    SbObject *parts;
    Sb_ssize_t pos, item_count;
    Sb_ssize_t posarg_counter;

    items = formatter_parse(self);
    parts = SbList_New(0);

    posarg_counter = 0;
    /* Relies on items being a list. */
    item_count = SbList_GetSizeUnsafe(items);
    while (pos < item_count) {
        SbObject *item;
        SbObject *o_literal, *o_name, *o_spec, *o_conv;

        item = SbList_GetItemUnsafe(items, pos);
        o_name = SbTuple_GetItemUnsafe(item, 1);
        o_spec = SbTuple_GetItemUnsafe(item, 2);
        o_conv = SbTuple_GetItemUnsafe(item, 3);

        o_literal = SbTuple_GetItemUnsafe(item, 0);
        if (SbStr_GetSize(o_literal) > 0) {
            SbList_Append(parts, o_literal);
        }

        if (o_name != Sb_None) {
            SbObject *o;
            o = formatter_get_field(o_name, args, kwargs);
        }
    }

    SbErr_RaiseWithString(SbExc_SystemError, "not implemented");
    return NULL;
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

/* Type initializer */

static const SbCMethodDef str_methods[] = {
    { "__hash__", str_hash },
    { "__str__", str_str },
    { "__len__", str_len },
    { "__eq__", str_eq },
    { "__ne__", str_ne },
    { "__getitem__", str_getitem },

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
