#include "snakebed.h"

/* Define the int object structure. */
typedef struct _SbIntObject {
    SbObject_HEAD;
    long value;
} SbIntObject;

/* Keep the type object here. */
SbTypeObject *SbInt_Type = NULL;

/*
 * C interface implementations
 */

int
SbInt_CheckExact(SbObject *op)
{
    return Sb_TYPE(op) == SbInt_Type;
}

long
SbInt_GetMax(void)
{
    return LONG_MAX;
}

SbObject *
SbInt_FromLong(long ival)
{
    SbIntObject *op;
    op = (SbIntObject *)SbObject_New(SbInt_Type);
    if (op) {
        op->value = ival;
    }
    return (SbObject *)op;
}

long
SbInt_AsLong(SbObject *op)
{
    return ((SbIntObject *)op)->value;
}

static long
convert_digit(char ch, int base)
{
    long value;
    
    value = ch - 0x30;
    if (value < (base < 10 ? base : 10))
        return value;
    value = (ch | 0x20) - 0x61 + 10;
    if (value >= base)
        return -1;
    return value;
}

SbObject *
SbInt_FromString(const char *str, const char **pend, unsigned base)
{
    long value = 0;
    const char *str_start;

    if (base == 0) {
        base = 10;
        if (str[0] == '0') {
            if (str[1] == 'x' || str[1] == 'X') {
                base = 16;
                str += 2;
            }
            else {
                base = 8;
                str += 1;
            }
        }
    }
    else if (base > 36 || base < 2) {
        /* raise ValueError */
        return NULL;
    }

    while (*str && *str == ' ' || *str == '\t') {
        ++str;
    }
    str_start = str;

    while (*str) {
        long digit = convert_digit(*str, base);
        if (digit < 0)
            break;
        value = value * base + digit;
        ++str;
    }

    if (str_start == str) {
        /* raise ValueError */
        return NULL;
    }

    if (pend) {
        *pend = str;
    }

    return SbInt_FromLong(value);
}

/* Python accessible methods */

static long
int_hash(SbIntObject *self)
{
    return self->value;
}

/* Builtins initializer */
int
_SbInt_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("int", NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbIntObject);

    SbInt_Type = tp;
    return 0;
}
