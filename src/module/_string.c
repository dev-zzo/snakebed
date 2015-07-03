#include "snakebed.h"
#include "internal.h"

SbObject *Sb_ModuleString = NULL;

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
            ++cursor;
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

    Sb_INCREF(o);
    return o;

empty_attr:
    SbErr_RaiseWithString(SbExc_ValueError, "empty attribute in field name");
    return NULL;
}

static SbObject *
formatter_convert(SbObject *o, SbObject *conv)
{
    char conv_type;

    conv_type = SbStr_AsStringUnsafe(conv)[0];
    switch (conv_type) {
    case 's':
        return SbObject_Str(o);
    case 'r':
        return SbObject_Repr(o);
    default:
        Sb_INCREF(o);
        return o;
    }
}

static SbObject *
formatter_format_value(SbObject *o, SbObject *spec)
{
    return SbObject_CallMethodObjArgs(o, "__format__", 1, spec);
}

SbObject *
Formatter_VFormat(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *items;
    SbObject *parts;
    SbObject *result;
    Sb_ssize_t pos, item_count;
    Sb_ssize_t posarg_counter;

    items = formatter_parse(self);
    parts = SbList_New(0);

    posarg_counter = 0;
    pos = 0;
    /* Relies on items being a list. */
    item_count = SbList_GetSizeUnsafe(items);
    while (pos < item_count) {
        SbObject *item;
        SbObject *o_literal, *o_name, *o_spec, *o_conv;

        item = SbList_GetItemUnsafe(items, pos++);
        o_name = SbTuple_GetItemUnsafe(item, 1);
        o_spec = SbTuple_GetItemUnsafe(item, 2);
        o_conv = SbTuple_GetItemUnsafe(item, 3);

        o_literal = SbTuple_GetItemUnsafe(item, 0);
        if (SbStr_GetSize(o_literal) > 0) {
            SbList_Append(parts, o_literal);
        }

        if (o_name != Sb_None) {
            SbObject *o;
            SbObject *o_converted;

            o = formatter_get_field(o_name, args, kwargs);
            if (!o) {
                goto error;
            }

            o_converted = formatter_convert(o, o_conv);
            Sb_DECREF(o);

            o = formatter_format_value(o_converted, o_spec);
            Sb_DECREF(o_converted);
            if (!o) {
                goto error;
            }
            SbList_Append(parts, o);
        }
    }
    Sb_DECREF(items);

    result = SbStr_Join(SbStr_FromString(""), parts);
    Sb_DECREF(parts);
    return result;

error:
    Sb_DECREF(items);
    Sb_DECREF(parts);
    return NULL;
}

#endif /* SUPPORTS(STR_FORMAT) */


static SbObject *
_Sb_TypeInit_Formatter(SbObject *m)
{
    SbTypeObject *tp;
    static const SbCMethodDef formatter_methods[] = {
        /* Sentinel */
        { NULL, NULL },
    };


    tp = _SbType_FromCDefs("Formatter", NULL, formatter_methods, sizeof(SbObject));
    if (!tp) {
        return NULL;
    }

    return (SbObject *)tp;
}

int
_Sb_ModuleInit_String()
{
    SbObject *m;
    SbObject *dict;
    SbObject *tp;

    m = Sb_InitModule("_string");
    if (!m) {
        return -1;
    }

    dict = SbModule_GetDict(m);
    if (!dict) {
        return -1;
    }

    tp = _Sb_TypeInit_Formatter(m);
    if (!tp) {
        Sb_DECREF(m);
        return -1;
    }
    SbDict_SetItemString(dict, "Formatter", tp);

    Sb_ModuleString = m;
    return 0;
}

void
_Sb_ModuleFini_String()
{
    Sb_CLEAR(Sb_ModuleString);
}
