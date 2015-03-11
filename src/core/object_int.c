#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbInt_Type = NULL;

/*
 * C interface implementations
 */

long
SbInt_GetMax(void)
{
    return LONG_MAX;
}

long
SbInt_GetMin(void)
{
    return LONG_MIN;
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
    if (!SbInt_Check(op)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-int object passed to an int method");
        return -1;
    }

    return SbInt_AsLongUnsafe(op);
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

int
SbInt_CompareBool(SbObject *p1, SbObject *p2, SbObjectCompareOp op)
{
    long v1, v2;

    if (!SbInt_CheckExact(p1) || !SbInt_CheckExact(p2)) {
        /* raise TypeError */
        return -1;
    }

    v1 = SbInt_AsLong(p1);
    v2 = SbInt_AsLong(p2);

    switch(op) {
    case Sb_LT:
        return v1 < v2;
    case Sb_LE:
        return v1 <= v2;
    case Sb_EQ:
        return v1 == v2;
    case Sb_NE:
        return v1 != v2;
    case Sb_GT:
        return v1 > v2;
    case Sb_GE:
        return v1 >= v2;
    }

    return -1;
}

/* Python accessible methods */

static SbObject *
int_init(SbIntObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *x = NULL;
    SbObject *base = NULL;

    if (SbTuple_Unpack(args, 0, 2, &x, &base) < 0) {
        return NULL;
    }

    if (x) {
        if (SbInt_CheckExact(x)) {
            self->value = SbInt_AsLongUnsafe(x);
        }
        else if (SbStr_CheckExact(x)) {
            /* TODO */
        }
    }

    Sb_RETURN_NONE;
}

static SbObject *
int_hash(SbObject *self, SbObject *args, SbObject *kwargs)
{
    Sb_INCREF(self);
    return self;
}

static SbObject *
int_nonzero(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbBool_FromLong(SbInt_AsLongUnsafe(self));
}

static SbObject *
int_compare_wrap(SbObject *self, SbObject *args, SbObjectCompareOp op)
{
    SbObject *other;
    int result;

    other = SbTuple_GetItem(args, 0);
    if (!other) {
        return NULL;
    }
    result = SbInt_CompareBool(self, other, op);
    if (result >= 0) {
        return SbBool_FromLong(result);
    }
    Sb_INCREF(Sb_NotImplemented);
    return Sb_NotImplemented;
}

static SbObject *
int_lt(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return int_compare_wrap(self, args, Sb_LT);
}

static SbObject *
int_le(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return int_compare_wrap(self, args, Sb_LE);
}

static SbObject *
int_eq(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return int_compare_wrap(self, args, Sb_EQ);
}

static SbObject *
int_ne(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return int_compare_wrap(self, args, Sb_NE);
}

static SbObject *
int_gt(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return int_compare_wrap(self, args, Sb_GT);
}

static SbObject *
int_ge(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return int_compare_wrap(self, args, Sb_GE);
}

static SbObject *
int_arith_wrap_binary(SbObject *self, SbObject *args, long (*func)(long lhs, long rhs))
{
    SbObject *other;

    other = SbTuple_GetItem(args, 0);
    if (!other) {
        return NULL;
    }

    if (!SbInt_CheckExact(other)) {
        Sb_INCREF(Sb_NotImplemented);
        return Sb_NotImplemented;
    }

    return SbInt_FromLong(func(SbInt_AsLong(self), SbInt_AsLong(other)));
}

static long
int_add_func(long lhs, long rhs)
{
    return lhs + rhs;
}

static SbObject *
int_add(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return int_arith_wrap_binary(self, args, int_add_func);
}

static long
int_sub_func(long lhs, long rhs)
{
    return lhs - rhs;
}

static SbObject *
int_sub(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return int_arith_wrap_binary(self, args, int_sub_func);
}

static SbObject *
int_mul(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *other;
    Sb_long64_t result;

    other = SbTuple_GetItem(args, 0);
    if (!other) {
        return NULL;
    }

    if (!SbInt_CheckExact(other)) {
        Sb_INCREF(Sb_NotImplemented);
        return Sb_NotImplemented;
    }

    result = Sb_Mul32x32As64(SbInt_AsLongUnsafe(self), SbInt_AsLongUnsafe(other));
    if (result > SbInt_GetMax() || result < SbInt_GetMin()) {
        /* TODO: Convert result to long */
        return NULL;
    }
    return SbInt_FromLong((long)result);
}

static long
int_fdiv_func(long lhs, long rhs)
{
    return lhs / rhs;
}

static SbObject *
int_fdiv(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return int_arith_wrap_binary(self, args, int_fdiv_func);
}

static long
int_shl_func(long lhs, long rhs)
{
    return lhs << rhs;
}

static SbObject *
int_shl(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return int_arith_wrap_binary(self, args, int_shl_func);
}

static long
int_shr_func(long lhs, long rhs)
{
    return lhs >> rhs;
}

static SbObject *
int_shr(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return int_arith_wrap_binary(self, args, int_shr_func);
}

static SbObject *
int_neg(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbInt_FromLong(-SbInt_AsLongUnsafe(self));
}

static SbObject *
int_pos(SbObject *self, SbObject *args, SbObject *kwargs)
{
    Sb_INCREF(self);
    return self;
}

static SbObject *
int_abs(SbObject *self, SbObject *args, SbObject *kwargs)
{
    if (SbInt_AsLongUnsafe(self) < 0) {
        return SbInt_FromLong(-SbInt_AsLongUnsafe(self));
    }
    Sb_INCREF(self);
    return self;
}

static SbObject *
int_inv(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbInt_FromLong(~SbInt_AsLongUnsafe(self));
}

/* Builtins initializer */

static const SbCMethodDef int_methods[] = {
    { "__init__", (SbCFunction)int_init },
    { "__hash__", int_hash },
    { "__nonzero__", int_nonzero },

    { "__lt__", int_lt },
    { "__le__", int_le },
    { "__eq__", int_eq },
    { "__ne__", int_ne },
    { "__gt__", int_gt },
    { "__ge__", int_ge },

    { "__add__", int_add },
    { "__iadd__", int_add },
    { "__sub__", int_sub },
    { "__isub__", int_sub },
    { "__mul__", int_mul },
    { "__imul__", int_mul },
    { "__floordiv__", int_fdiv },
    { "__ifloordiv__", int_fdiv },
    { "__lshift__", int_shl },
    { "__ilshift__", int_shl },
    { "__rshift__", int_shr },
    { "__irshift__", int_shr },

    { "__neg__", int_neg },
    { "__pos__", int_pos },
    { "__abs__", int_abs },
    { "__invert__", int_inv },

    /* Sentinel */
    { NULL, NULL },
};

int
_SbInt_BuiltinInit()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("int", NULL, int_methods, sizeof(SbIntObject));
    if (!tp) {
        return -1;
    }
    SbInt_Type = tp;
    return 0;
}
