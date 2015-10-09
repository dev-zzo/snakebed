#include "snakebed.h"
#include "internal.h"

/* https://www.python.org/dev/peps/pep-0237/ */

/* Keep the type object here. */
SbTypeObject *SbInt_Type = NULL;

/*
Implementation of multiple-precision arithmetic.

The implementation uses 2-complement representation of digits.

The implementation is defined to operate on half-words to allow for:
* Easy carry/borrow computations, trading off for speed (2x addition ops).
* Easy multiplication for machines not having a high-bits output reg.

Some algorithms are taken from the awesome Hacker's Delight book.
*/

/* MACRO: test if the MPI is negative. */
#define LONG_IS_NEGATIVE(o) \
    (((SbInt_SignDigit_t)o->u.digits[o->length - 1]) < 0)

/* Obtain the digit filled with the sign bit.
   Returns: -1 or 0 depending on the sign. */
static SbInt_Digit_t
long_sign(const SbInt_Value *o)
{
    return -LONG_IS_NEGATIVE(o);
}

/* Compare two MPI values.
   Returns: negative if lhs < rhs, zero if lhs == rhs, positive if lhs > rhs. */
static int
long_cmp(const SbInt_Value *lhs, const SbInt_Value *rhs)
{
    int sign;
    Sb_ssize_t diff;
    long i;

    /* Fast path:
       lhs < 0 && rhs > 0 => -1
       lhs > 0 && rhs < 0 => +1
    */
    sign = LONG_IS_NEGATIVE(lhs);
    diff = LONG_IS_NEGATIVE(rhs) - sign;
    if (diff) {
        return diff;
    }
    /* Established: lhs and rhs have the same sign. */

    /* Fast path: if lengths are different, no need to compare digits. */
    diff = lhs->length - rhs->length;
    if (diff) {
        /* Both negative: longer one is less */
        /* Both positive: shorter one is less. */
        return sign ? -diff : diff;
    }
    /* Established: lhs and rhs have the same sign and the same length. */

    for (i = lhs->length - 1; i >= 0; --i) {
        diff = lhs->u.digits[i] != rhs->u.digits[i];
        if (diff) {
            /* Both positive: bigger one is +1. */
            /* Both negative: bigger one is -1. */
            return sign ? -diff : diff;
        }
    }
    /* lhs and rhs are bit-equal. */
    return 0;
}

/* Negate an MPI value.
   Assumes: result->length == rhs->length
   NOTE: Allows for operand aliasing. */
static void
__long_neg(const SbInt_Value *rhs, SbInt_Value *result)
{
    SbInt_Digit_t c;
    size_t i;

    c = 1;
    for (i = 0; i < result->length; ++i) {
        SbInt_DoubleDigit_t t;

        t = ~rhs->u.digits[i] + c;
        c = t >> SbInt_DIGIT_BITS;
        result->u.digits[i] = t & 0xFFFFu;
    }
}

/* Add two MPI values.
   Assumes: result->length > max(lhs->length, rhs->length)
   NOTE: Allows for aliasing all three ops. */
static void
__long_add(const SbInt_Value *lhs, const SbInt_Value *rhs, SbInt_Value *result)
{
    SbInt_Digit_t lhs_sign, rhs_sign;
    SbInt_Digit_t c;
    size_t i;

    lhs_sign = long_sign(lhs);
    rhs_sign = long_sign(rhs);

    c = 0;
    for (i = 0; i < result->length; ++i) {
        SbInt_Digit_t a, b;
        SbInt_DoubleDigit_t t;

        a = i < lhs->length ? lhs->u.digits[i] : lhs_sign;
        b = i < rhs->length ? rhs->u.digits[i] : rhs_sign;

        t = a + b + c;
        c = t >> SbInt_DIGIT_BITS;
        result->u.digits[i] = t & 0xFFFFu;
    }
}

/* Subtract two MPI values.
   Assumes: result->length > max(lhs->length, rhs->length)
   NOTE: Allows for aliasing all three ops. */
static void
__long_sub(const SbInt_Value *lhs, const SbInt_Value *rhs, SbInt_Value *result)
{
    SbInt_Digit_t lhs_sign, rhs_sign;
    SbInt_Digit_t c;
    size_t i;

    lhs_sign = long_sign(lhs);
    rhs_sign = long_sign(rhs);

    c = 0;
    for (i = 0; i < result->length; ++i) {
        SbInt_Digit_t a, b;
        SbInt_DoubleDigit_t t;

        a = i < lhs->length ? lhs->u.digits[i] : lhs_sign;
        b = i < rhs->length ? rhs->u.digits[i] : rhs_sign;

        t = a - b - c;
        c = t >> SbInt_DIGIT_BITS;
        result->u.digits[i] = t & 0xFFFFu;
    }
}

/* Multiply two MPI values.
   Assumes: result->length == lhs->length + rhs->length
   NOTE: Does NOT allow aliasing. */
static void
__long_mul(const SbInt_Value *lhs, const SbInt_Value *rhs, SbInt_Value *result)
{
    size_t i, j;

    for (i = 0; i < result->length; ++i) {
        result->u.digits[i] = 0;
    }

    for (i = 0; i < lhs->length; ++i) {
        SbInt_Digit_t a;
        SbInt_Digit_t c;

        a = lhs->u.digits[i];
        /* Fast path: multiplying by zero digit requires no additions */
        if (!a) {
            continue;
        }
        c = 0;
        for (j = 0; j < rhs->length; ++j) {
            SbInt_Digit_t b;
            SbInt_DoubleDigit_t t;

            b = rhs->u.digits[j];
            t = a * b + result->u.digits[i + j] + c;
            c = t >> SbInt_DIGIT_BITS;
            result->u.digits[i + j] = t & 0xFFFFu;
        }
        result->u.digits[i + j] = c;
    }
    /* subtracting v * 2**16m if u < 0; inlined */
    if (long_sign(lhs)) {
        SbInt_Digit_t b;
        SbInt_DoubleDigit_t t;

        for (i = 0; 0 < rhs->length; ++i) {
            t = result->u.digits[lhs->length + i] - rhs->u.digits[i] - b;
            result->u.digits[lhs->length + i] = t & 0xFFFFu;
            b = t >> 31;
        }
    }
    /* subtracting u * 2**16n if v < 0; inlined */
    if (long_sign(rhs)) {
        SbInt_Digit_t b;
        SbInt_DoubleDigit_t t;

        for (i = 0; 0 < lhs->length; ++i) {
            t = result->u.digits[rhs->length + i] - lhs->u.digits[i] - b;
            result->u.digits[rhs->length + i] = t & 0xFFFFu;
            b = t >> 31;
        }
    }
}



/*
 * C interface implementations
 */

void
_SbInt_SetFromNative(SbObject *self, SbInt_Native_t ival)
{
    SbIntObject *myself = (SbIntObject *)self;

    myself->v.native = 1;
    myself->v.u.value = ival;
}

SbObject *
SbInt_FromNative(SbInt_Native_t ival)
{
    SbIntObject *myself;
    myself = (SbIntObject *)SbObject_New(SbInt_Type);
    if (myself) {
        _SbInt_SetFromNative((SbObject *)myself, ival);
    }
    return (SbObject *)myself;
}

SbInt_Native_t
SbInt_AsNativeOverflow(SbObject *op, int *overflow_flag)
{
    const SbIntObject * myself;

#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbInt_Check(op)) {
        SbErr_RaiseWithString(SbExc_SystemError, "non-int object passed to an int method");
        return -1;
    }
#endif


    myself = (SbIntObject *)op;
    if (myself->v.native) {
        if (overflow_flag) {
            *overflow_flag = 0;
        }
        return myself->v.u.value;
    }
    /* TODO: allow for more flexibility wrt SbInt_DIGIT_BITS? */
    if (myself->v.length < 3) {
        SbInt_DoubleDigit_t t;

        t = myself->v.u.digits[0] | ((SbInt_DoubleDigit_t)myself->v.u.digits[1] << SbInt_DIGIT_BITS);
        if (overflow_flag) {
            *overflow_flag = 0;
        }
        return t;
    }
    /* Can't be represented. */
    if (overflow_flag) {
        *overflow_flag = 1;
    }
    return -1;
}

SbInt_Native_t
SbInt_AsNative(SbObject *op)
{
    SbInt_Native_t result;
    int overflow_flag;

    result = SbInt_AsNativeOverflow(op, &overflow_flag);
    if (result == -1 && overflow_flag) {
    }
    return result;
}

static void
long_convert_digits(SbInt_Value *value, SbInt_Native_t native_value, Sb_size_t digits_length, SbInt_Digit_t *digits)
{
    value->native = 0;
    value->length = digits_length;
    value->u.digits = digits;

    /* NOTE: Do away with the loop and just stick to SbInt_DIGIT_BITS being 16? */
    while (digits_length-- > 0) {
        *digits++ = native_value & 0xFFFFu;
        native_value >>= SbInt_DIGIT_BITS;
    }
}

typedef int (*long_ivv)(const SbInt_Value *lhs, const SbInt_Value *rhs);
typedef int (*long_vvv)(const SbInt_Value *lhs, const SbInt_Value *rhs);

static int
long_coerce_apply_ivv(SbInt_Value *lhs, SbInt_Value *rhs, long_ivv func)
{
    SbInt_Value lhs_copy;
    SbInt_Value rhs_copy;
    SbInt_Digit_t lhs_digits[2];
    SbInt_Digit_t rhs_digits[2];

    long_convert_digits(&lhs_copy, lhs->u.value, 2, lhs_digits);
    long_convert_digits(&rhs_copy, rhs->u.value, 2, rhs_digits);
    return func(&lhs_copy, &rhs_copy);
}

int
SbInt_CompareBool(SbObject *p1, SbObject *p2, SbObjectCompareOp op)
{
    SbIntObject *i1, *i2;
    SbInt_Native_t v1, v2;

    if (!SbInt_Check(p1) || !SbInt_Check(p2)) {
        SbErr_RaiseWithString(SbExc_SystemError, "non-int object passed to an int method");
        return -1;
    }

    i1 = (SbIntObject *)p1;
    i2 = (SbIntObject *)p2;
    if (i1->v.native && i2->v.native) {
        v1 = i1->v.u.value;
        v2 = i2->v.u.value;
    }
    else {
        v1 = long_coerce_apply_ivv(&i1->v, &i2->v, long_cmp);
        v2 = 0;
    }

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
    int radix = 0;

    if (SbArgs_Parse("|O:x,i:radix", args, kwargs, &x, &radix) < 0) {
        return NULL;
    }

    if (x) {
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
    return NULL;
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
int_add(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return NULL;
}

static SbObject *
int_sub(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return NULL;
}

static SbObject *
int_mul(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return NULL;
}

static SbObject *
int_fdiv(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return NULL;
}

static SbObject *
int_shl(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return NULL;
}

static SbObject *
int_shr(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return NULL;
}

static SbObject *
int_neg(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return NULL;
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
    return NULL;
}

static SbObject *
int_inv(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return NULL;
}

#if SUPPORTS(STR_FORMAT)

static SbObject *
int_format(SbObject *self, SbObject *args, SbObject *kwargs)
{
    const char *spec_string;
    SbString_FormatSpecifier spec;
    SbObject *tmp;
    SbObject *o_result;

    if (SbArgs_Parse("z:spec", args, kwargs, &spec_string) < 0) {
        return NULL;
    }
    if (SbString_ParseFormatSpec(spec_string ? spec_string : "", &spec) < 0) {
        return NULL;
    }

    if (spec.conv_type == 'c') {
        SbInt_Native_t value;

        value = SbInt_AsNative(self);
        if (value == -1 && SbErr_Occurred()) {
            return NULL;
        }
        else if (value > 0xFF || value < 0) {
            SbErr_RaiseWithString(SbExc_ValueError, "can't represent this as a character");
            return NULL;
        }

        tmp = SbStr_FromStringAndSize(NULL, 1);
        SbStr_AsStringUnsafe(tmp)[0] = (char)value;
    }
    else {
        /* NOTE: Needs to be redone completely. */
        SbInt_Native_t value;
        int radix = 10;
        char sign = '+';
        Sb_ssize_t sign_size;
        Sb_ssize_t fill_size;
        const char *digits;
        Sb_ssize_t digits_size;
        char *buffer;

        value = SbInt_AsNative(self);
        if (value < 0) {
            value = -value;
            sign = '-';
        }

        if (spec.sign_flag == '+') {
            sign_size = 1;
        }
        else if (spec.sign_flag == ' ') {
            sign_size = 1;
            if (sign == '+') {
                sign = ' ';
            }
        }
        else {
            sign_size = sign == '-' ? 1 : 0;
        }

        switch (spec.conv_type) {
        case 'x':
        case 'X':
            radix = 16;
            break;
        case 'b':
            radix = 2;
            break;
        case 'o':
            radix = 8;
            break;
        case 'd':
        case 'n':
        case '\0':
            break;
        default:
            break;
        }
        digits = Sb_ULtoA(value, radix);
        digits_size = SbRT_StrLen(digits);

        fill_size = spec.min_width - (sign_size + digits_size);
        if (fill_size < 0) {
            fill_size = 0;
        }

        if (spec.align_flag == '=') {
            /* [sign][fill]digits */
            o_result = SbStr_FromStringAndSize(NULL, sign_size + digits_size + fill_size);
            buffer = SbStr_AsStringUnsafe(o_result);
            if (sign_size) {
                *buffer++ = sign;
            }
            while (fill_size > 0) {
                *buffer++ = spec.filler;
                fill_size--;
            }
            SbRT_MemCpy(buffer, digits, digits_size);
            return o_result;
        }

        tmp = SbStr_FromStringAndSize(NULL, sign_size + digits_size);
        buffer = SbStr_AsStringUnsafe(tmp);
        if (sign_size) {
            *buffer++ = sign;
        }
        SbRT_MemCpy(buffer, digits, digits_size);
    }

    if (spec.align_flag == '>') {
        o_result = SbStr_JustifyRight(tmp, spec.min_width, spec.filler);
    }
    else if (spec.align_flag == '^') {
        o_result = SbStr_JustifyCenter(tmp, spec.min_width, spec.filler);
    }
    else if (spec.align_flag == '<') {
        o_result = SbStr_JustifyLeft(tmp, spec.min_width, spec.filler);
    }
    Sb_DECREF(tmp);
    return o_result;
}

#endif /* SUPPORTS(STR_FORMAT) */

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

#if SUPPORTS(STR_FORMAT)
    { "__format__", int_format },
#endif

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
