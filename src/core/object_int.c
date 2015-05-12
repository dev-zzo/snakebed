#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbInt_Type = NULL;

/*
 * C interface implementations
 */

SbInt_Native_t
SbInt_GetMax(void)
{
    return LONG_MAX;
}

SbInt_Native_t
SbInt_GetMin(void)
{
    return LONG_MIN;
}

SbObject *
SbInt_FromNative(SbInt_Native_t ival)
{
    SbIntObject *op;
    op = (SbIntObject *)SbObject_New(SbInt_Type);
    if (op) {
        op->value = ival;
    }
    return (SbObject *)op;
}

SbInt_Native_t
SbInt_AsNative(SbObject *op)
{
#if SUPPORTS_BUILTIN_TYPECHECKS
    if (!SbInt_Check(op)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-int object passed to an int method");
        return -1;
    }
#endif

    return SbInt_AsNativeUnsafe(op);
}

static SbInt_Native_t
convert_digit(char ch, int base)
{
    SbInt_Native_t value;
    
    value = ch - 0x30;
    if (value < (base < 10 ? base : 10))
        if (value < 0)
            return -1;
        return value;
    value = (ch | 0x20) - 0x61 + 10;
    if (value >= base)
        return -1;
    return value;
}

static int
int_parse_string(const char *str, const char **pend, unsigned base, SbInt_Native_t *result)
{
    SbInt_Native_t value = 0;
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
        SbErr_RaiseWithString(SbErr_ValueError, "incorrect `base` value (expected 2<=base<=36)");
        return -1;
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
        SbErr_RaiseWithString(SbErr_ValueError, "incorrect input string");
        return -1;
    }

    if (pend) {
        *pend = str;
    }

    if (result) {
        *result = value;
    }
    return 0;
}

SbObject *
SbInt_FromString(const char *str, const char **pend, unsigned base)
{
    SbInt_Native_t value;

    if (int_parse_string(str, pend, base, &value) < 0) {
        return NULL;
    }

    return SbInt_FromNative(value);
}

int
_SbInt_Convert(SbObject *o, SbObject *base, SbInt_Native_t *value)
{
    if (SbInt_CheckExact(o)) {
        *value = SbInt_AsNativeUnsafe(o);
        return 0;
    }
    if (SbStr_CheckExact(o)) {
        const char *str;
        SbInt_Native_t base_conv = 0;

        str = (const char *)SbStr_AsStringUnsafe(o);
        if (base) {
            /* TODO: Try converting to int first? */
            if (!SbInt_CheckExact(base)) {
                SbErr_RaiseWithString(SbErr_ValueError, "incorrect `base` type");
                return -1;
            }
            base_conv = SbInt_AsNativeUnsafe(base);
        }

        if (int_parse_string(str, NULL, base_conv, value) < 0) {
            return -1;
        }
        return 0;
    }
    /* if (SbLong_CheckExact(o)) ... */
    /* if (SbFloat_CheckExact(o)) ... */
    SbErr_RaiseWithString(SbErr_ValueError, "cannot convert the object to an int");
    return -1;
}

int
SbInt_CompareBool(SbObject *p1, SbObject *p2, SbObjectCompareOp op)
{
    SbInt_Native_t v1, v2;

    if (!SbInt_CheckExact(p1) || !SbInt_CheckExact(p2)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-int object passed to an int method");
        return -1;
    }

    v1 = SbInt_AsNativeUnsafe(p1);
    v2 = SbInt_AsNativeUnsafe(p2);

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

    if (SbArgs_Unpack(args, 0, 2, &x, &base) < 0) {
        return NULL;
    }

    if (x) {
        if (_SbInt_Convert(x, base, &self->value) < 0) {
            return NULL;
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
    return SbBool_FromLong(SbInt_AsNativeUnsafe(self));
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
int_arith_wrap_binary(SbObject *self, SbObject *args, SbInt_Native_t (*func)(SbInt_Native_t lhs, SbInt_Native_t rhs))
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

    return SbInt_FromNative(func(SbInt_AsNative(self), SbInt_AsNative(other)));
}

static SbInt_Native_t
int_add_func(SbInt_Native_t lhs, SbInt_Native_t rhs)
{
    return lhs + rhs;
}

static SbObject *
int_add(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return int_arith_wrap_binary(self, args, int_add_func);
}

static SbInt_Native_t
int_sub_func(SbInt_Native_t lhs, SbInt_Native_t rhs)
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

    result = Sb_Mul32x32As64(SbInt_AsNativeUnsafe(self), SbInt_AsNativeUnsafe(other));
    if (result > SbInt_GetMax() || result < SbInt_GetMin()) {
        /* TODO: Convert result to long */
        return NULL;
    }
    return SbInt_FromNative((SbInt_Native_t)result);
}

static SbInt_Native_t
int_fdiv_func(SbInt_Native_t lhs, SbInt_Native_t rhs)
{
    return lhs / rhs;
}

static SbObject *
int_fdiv(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return int_arith_wrap_binary(self, args, int_fdiv_func);
}

static SbInt_Native_t
int_shl_func(SbInt_Native_t lhs, SbInt_Native_t rhs)
{
    return lhs << rhs;
}

static SbObject *
int_shl(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return int_arith_wrap_binary(self, args, int_shl_func);
}

static SbInt_Native_t
int_shr_func(SbInt_Native_t lhs, SbInt_Native_t rhs)
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
    return SbInt_FromNative(-SbInt_AsNativeUnsafe(self));
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
    if (SbInt_AsNativeUnsafe(self) < 0) {
        return SbInt_FromNative(-SbInt_AsNativeUnsafe(self));
    }
    Sb_INCREF(self);
    return self;
}

static SbObject *
int_inv(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbInt_FromNative(~SbInt_AsNativeUnsafe(self));
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
