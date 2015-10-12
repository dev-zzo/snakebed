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

static SbInt_Digit_t *
long_alloc(Sb_size_t count)
{
    return Sb_Calloc(count, sizeof(SbInt_Digit_t));
}

static void
long_free(SbInt_Digit_t *digits)
{
    Sb_Free(digits);
}

#define LONG_IS_NATIVE(o) \
    ((o)->length <= 0)
#define LONG_SET_NATIVE(o) \
    ((o)->length = -1)

/* MACRO: test if the MPI is negative. */
#define LONG_IS_NEGATIVE(o) \
    (((SbInt_SignDigit_t)(o)->u.digits[(o)->length - 1]) < 0)

/* MACRO: test if the MPI is zero. */
#define LONG_IS_ZERO(o) \
    (LONG_IS_NATIVE(o) && ((o)->u.value == 0))

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
    SbInt_Digit_t *src, *dst, *dst_limit;

    src = rhs->u.digits;
    dst = result->u.digits;
    dst_limit = dst + result->length;

    c = 1;
    while (dst < dst_limit) {
        SbInt_DoubleDigit_t t;

        t = ~(*src++) + c;
        c = t >> SbInt_DIGIT_BITS;
        (*dst++) = t & 0xFFFFu;
    }
}

/* Invert an MPI value.
   Assumes: result->length == rhs->length
   NOTE: Allows for operand aliasing. */
static void
__long_inv(const SbInt_Value *rhs, SbInt_Value *result)
{
    SbInt_Digit_t *src, *dst, *dst_limit;

    src = rhs->u.digits;
    dst = result->u.digits;
    dst_limit = dst + result->length;

    while (dst < dst_limit) {
        (*dst++) = ~(*src++);
    }
}

/* Add two MPI values.
   Assumes: result->length > max(lhs->length, rhs->length)
   NOTE: Allows for aliasing all three ops. */
static void
__long_add(const SbInt_Value *lhs, const SbInt_Value *rhs, SbInt_Value *result)
{
    SbInt_Digit_t *src1, *src2, *limit1, *limit2, *dst;
    SbInt_Digit_t sign;
    SbInt_DoubleDigit_t c;

    if (lhs->length < rhs->length) {
        const SbInt_Value *tmp;

        tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    src1 = lhs->u.digits;
    limit1 = src1 + lhs->length;
    src2 = rhs->u.digits;
    limit2 = src2 + rhs->length;
    dst = result->u.digits;

    c = 0;
    while (src2 < limit2) {
        SbInt_DoubleDigit_t t;

        t = *src1 + *src2 + c;
        c = t >> SbInt_DIGIT_BITS;
        *dst = t & 0xFFFFu;
        ++src1;
        ++src2;
        ++dst;
    }

    sign = ((SbInt_SignDigit_t)src2[-1]) >> (SbInt_DIGIT_BITS - 1);
    while (src1 < limit1) {
        SbInt_DoubleDigit_t t;

        t = *src1 + sign + c;
        c = t >> SbInt_DIGIT_BITS;
        *dst = t & 0xFFFFu;
        ++src1;
        ++dst;
    }

    *dst = (SbInt_Digit_t)((((SbInt_SignDigit_t)src1[-1]) >> (SbInt_DIGIT_BITS - 1)) + sign + c);
}

/* Subtract two MPI values.
   Assumes: result->length > max(lhs->length, rhs->length)
   NOTE: Allows for aliasing all three ops. */
static void
__long_sub(const SbInt_Value *lhs, const SbInt_Value *rhs, SbInt_Value *result)
{
    SbInt_Digit_t *src1, *src2, *limit1, *limit2, *dst;
    SbInt_Digit_t sign;
    SbInt_DoubleDigit_t c;

    if (lhs->length < rhs->length) {
        const SbInt_Value *tmp;

        tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    src1 = lhs->u.digits;
    limit1 = src1 + lhs->length;
    src2 = rhs->u.digits;
    limit2 = src2 + rhs->length;
    dst = result->u.digits;

    c = 0;
    while (src2 < limit2) {
        SbInt_DoubleDigit_t t;

        t = *src1 - *src2 - c;
        c = t >> SbInt_DIGIT_BITS;
        *dst = t & 0xFFFFu;
        ++src1;
        ++src2;
        ++dst;
    }

    sign = ((SbInt_SignDigit_t)src2[-1]) >> (SbInt_DIGIT_BITS - 1);
    while (src1 < limit1) {
        SbInt_DoubleDigit_t t;

        t = *src1 - sign - c;
        c = t >> SbInt_DIGIT_BITS;
        *dst = t & 0xFFFFu;
        ++src1;
        ++dst;
    }

    *dst = (SbInt_Digit_t)((((SbInt_SignDigit_t)src1[-1]) >> (SbInt_DIGIT_BITS - 1)) - sign - c);
}

/* Multiply two MPI values.
   Assumes: result->length == lhs->length + rhs->length
   NOTE: Does NOT allow aliasing. */
static void
__long_mul(const SbInt_Value *lhs, const SbInt_Value *rhs, SbInt_Value *result)
{
    Sb_ssize_t i, j;

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

        b = 0;
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

        b = 0;
        for (i = 0; 0 < lhs->length; ++i) {
            t = result->u.digits[rhs->length + i] - lhs->u.digits[i] - b;
            result->u.digits[rhs->length + i] = t & 0xFFFFu;
            b = t >> 31;
        }
    }
}

static void
long_reduce(SbInt_Value *o)
{
    SbInt_Digit_t sign;
    Sb_ssize_t i;

    if (LONG_IS_NATIVE(o)) {
        return;
    }

    sign = long_sign(o);
    i = o->length - 1;
    if (o->u.digits[i] != sign) {
        return;
    }
    do {
        --i;
    } while ((i >= 0) && (o->u.digits[i] == sign));
    if ((i >= 0) && ((o->u.digits[i] ^ sign) & (1 << (SbInt_DIGIT_BITS - 1)))) {
        ++i;
    }

    if (i < 2) {
        SbInt_Native_t v;

        if (i == 1) {
            v = (o->u.digits[1] << SbInt_DIGIT_BITS) | (o->u.digits[0]);
        }
        else if (i == 0) {
            v = (sign << SbInt_DIGIT_BITS) | (o->u.digits[0]);
        }
        else {
            v = (sign << SbInt_DIGIT_BITS) | (sign);
        }

        long_free(o->u.digits);
        LONG_SET_NATIVE(o);
        o->u.value = v;
    }
    else {
        o->length = i + 1;
    }
}


/*
 * C interface implementations
 */

void
_SbInt_SetFromNative(SbObject *self, SbInt_Native_t ival)
{
    SbIntObject *myself = (SbIntObject *)self;

    LONG_SET_NATIVE(&myself->v);
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

SbObject *
SbInt_FromLengthAndDigits(Sb_ssize_t length, SbInt_Digit_t *digits)
{
    SbIntObject *myself;
    myself = (SbIntObject *)SbObject_New(SbInt_Type);
    if (myself) {
        SbInt_Digit_t *new_digits;

        new_digits = long_alloc(length);
        if (!new_digits) {
            Sb_DECREF(myself);
            return NULL;
        }
        myself->v.length = length;
        myself->v.u.digits = new_digits;
        if (digits) {
            SbRT_MemCpy(new_digits, digits, length * sizeof(SbInt_Digit_t));
        }
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
    if (LONG_IS_NATIVE(&myself->v)) {
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
    value->length = digits_length;
    value->u.digits = digits;

    /* NOTE: Do away with the loop and just stick to SbInt_DIGIT_BITS being 16? */
    while (digits_length-- > 0) {
        *digits++ = native_value & 0xFFFFu;
        native_value >>= SbInt_DIGIT_BITS;
    }
}

typedef int (*long_ivv)(const SbInt_Value *lhs, const SbInt_Value *rhs);
typedef void (*long_vvr)(const SbInt_Value *lhs, const SbInt_Value *rhs, SbInt_Value *result);

static int
long_coerce_apply_ivv(const SbInt_Value *lhs, const SbInt_Value *rhs, long_ivv func)
{
    SbInt_Value lhs_copy;
    SbInt_Value rhs_copy;
    SbInt_Digit_t lhs_digits[2];
    SbInt_Digit_t rhs_digits[2];

    if (LONG_IS_NATIVE(lhs)) {
        long_convert_digits(&lhs_copy, lhs->u.value, 2, lhs_digits);
        lhs = &lhs_copy;
    }
    if (LONG_IS_NATIVE(rhs)) {
        long_convert_digits(&rhs_copy, rhs->u.value, 2, rhs_digits);
        rhs = &rhs_copy;
    }
    return func(lhs, rhs);
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
    if (LONG_IS_NATIVE(&i1->v) && LONG_IS_NATIVE(&i2->v)) {
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

static SbObject *
int_do_neginv(
    const SbInt_Value *val, 
    int (fnative)(const SbInt_Value *val, SbInt_Value *res),
    void (flong)(const SbInt_Value *val, SbInt_Value *res))
{
    SbIntObject *o_result;
    SbInt_Value *res;

    o_result = (SbIntObject *)SbObject_New(SbInt_Type);
    if (!o_result) {
        return NULL;
    }

    res = &o_result->v;
    LONG_SET_NATIVE(res);
    if (LONG_IS_NATIVE(val)) {
        if (fnative(val, res) < 0) {
            Sb_DECREF(o_result);
            return NULL;
        }
    }
    else {
        SbInt_Digit_t *digits;

        digits = long_alloc(val->length);
        if (!digits) {
            Sb_DECREF(o_result);
            return NULL;
        }
        res->length = val->length;
        res->u.digits = digits;
        flong(val, res);
    }
    /* No length correction required */
    return (SbObject *)o_result;
}

static int
int_negate_native(const SbInt_Value *val, SbInt_Value *res)
{
    if (SbInt_NATIVE_MIN == val->u.value) {
        SbInt_Digit_t *digits;

        digits = long_alloc(3);
        if (!digits) {
            return -1;
        }
        res->length = 3;
        res->u.digits = digits;
        digits[0] = (SbInt_Digit_t)SbInt_NATIVE_MIN;
        digits[1] = (SbInt_Digit_t)(SbInt_NATIVE_MIN >> SbInt_DIGIT_BITS);
        digits[2] = 0;
    }
    res->u.value = -val->u.value;
    return 0;
}

SbObject *
SbInt_Negate(SbObject *o)
{
    return int_do_neginv(&((SbIntObject *)o)->v, int_negate_native, __long_neg);
}

static int
int_invert_native(const SbInt_Value *val, SbInt_Value *res)
{
    res->u.value = ~val->u.value;
    return 0;
}

SbObject *
SbInt_Invert(SbObject *o)
{
    return int_do_neginv(&((SbIntObject *)o)->v, int_invert_native, __long_inv);
}

SbObject *
SbInt_Absolute(SbObject *o)
{
    const SbInt_Value *val = &((SbIntObject *)o)->v;

    if (LONG_IS_NATIVE(val)) {
        if (val->u.value < 0) {
            return SbInt_Negate(o);
        }
    }
    else {
        if (LONG_IS_NEGATIVE(val)) {
            return SbInt_Negate(o);
        }
    }
    Sb_INCREF(o);
    return o;
}

typedef int (*int_binary_op)(const SbInt_Value *lhs, const SbInt_Value *rhs, SbInt_Value *res);

static SbObject *
int_do_binary_op(const SbInt_Value *lhs, const SbInt_Value *rhs, int_binary_op fnative, int_binary_op flong)
{
    SbIntObject *result;
    SbInt_Value lhs_copy;
    SbInt_Value rhs_copy;
    SbInt_Digit_t lhs_digits[2];
    SbInt_Digit_t rhs_digits[2];

    result = (SbIntObject *)SbObject_New(SbInt_Type);
    if (!result) {
        return NULL;
    }
    LONG_SET_NATIVE(&result->v);

    if (LONG_IS_NATIVE(lhs) && LONG_IS_NATIVE(rhs)) {
        if (!fnative(lhs, rhs, &result->v)) {
            return (SbObject *)result;
        }
    }

    if (LONG_IS_NATIVE(lhs)) {
        long_convert_digits(&lhs_copy, lhs->u.value, 2, lhs_digits);
        lhs = &lhs_copy;
    }
    if (LONG_IS_NATIVE(rhs)) {
        long_convert_digits(&rhs_copy, rhs->u.value, 2, rhs_digits);
        rhs = &rhs_copy;
    }

    if (flong(lhs, rhs, &result->v) < 0) {
        Sb_DECREF(result);
        return NULL;
    }
    long_reduce(&result->v);
    return (SbObject *)result;
}

static int
int_add_native(const SbInt_Value *lhs, const SbInt_Value *rhs, SbInt_Value *res)
{
    SbInt_Native_t a, b, rv;

    a = lhs->u.value;
    b = rhs->u.value;
    res->u.value = rv = a + b;
    /* Signed integer overflow of addition occurs if and only if
    the operands have the same sign and 
    the sum has a sign opposite to that of the operands. */
    return (~(a ^ b) && ((rv ^ b) & 0x80000000U));
}

static int
int_add_long(const SbInt_Value *lhs, const SbInt_Value *rhs, SbInt_Value *res)
{
    Sb_ssize_t length;

    length = (lhs->length > rhs->length ? lhs->length : rhs->length) + 1;
    res->u.digits = long_alloc(length);
    if (!res->u.digits) {
        return -1;
    }
    res->length = length;
    __long_add(lhs, rhs, res);
    return 0;
}

SbObject *
SbInt_Add(SbObject *lhs, SbObject *rhs)
{
    return int_do_binary_op(&((SbIntObject *)lhs)->v, &((SbIntObject *)rhs)->v, int_add_native, int_add_long);
}

static int
int_sub_native(const SbInt_Value *lhs, const SbInt_Value *rhs, SbInt_Value *res)
{
    SbInt_Native_t a, b, rv;

    a = lhs->u.value;
    b = rhs->u.value;
    res->u.value = rv = a - b;
    return ((a ^ b) && ((rv ^ a) & 0x80000000U));
}

static int
int_sub_long(const SbInt_Value *lhs, const SbInt_Value *rhs, SbInt_Value *res)
{
    Sb_ssize_t length;

    length = (lhs->length > rhs->length ? lhs->length : rhs->length) + 1;
    res->u.digits = long_alloc(length);
    if (!res->u.digits) {
        return -1;
    }
    res->length = length;
    __long_sub(lhs, rhs, res);
    return 0;
}

SbObject *
SbInt_Subtract(SbObject *lhs, SbObject *rhs)
{
    return int_do_binary_op(&((SbIntObject *)lhs)->v, &((SbIntObject *)rhs)->v, int_sub_native, int_sub_long);
}

static int
int_mul_native(const SbInt_Value *lhs, const SbInt_Value *rhs, SbInt_Value *res)
{
    SbInt_Native_t a, b, rv;

    a = lhs->u.value;
    b = rhs->u.value;

    return 0;
}

static int
int_mul_long(const SbInt_Value *lhs, const SbInt_Value *rhs, SbInt_Value *res)
{
    Sb_ssize_t length;

    length = lhs->length + rhs->length;
    res->u.digits = long_alloc(length);
    if (!res->u.digits) {
        return -1;
    }
    res->length = length;
    __long_mul(lhs, rhs, res);
    return 0;
}

SbObject *
SbInt_Multiply(SbObject *lhs, SbObject *rhs)
{
    return int_do_binary_op(&((SbIntObject *)lhs)->v, &((SbIntObject *)rhs)->v, int_mul_native, int_mul_long);
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

    LONG_SET_NATIVE(&self->v);
    if (x) {
    }

    Sb_RETURN_NONE;
}

static void
int_destroy(SbIntObject *self)
{
    if (!LONG_IS_NATIVE(&self->v)) {
        long_free(self->v.u.digits);
    }
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
    const SbInt_Value *val = &((SbIntObject *)self)->v;

    if (LONG_IS_NATIVE(val) && val->u.value == 0) {
        Sb_RETURN_FALSE;
    }
    Sb_RETURN_TRUE;
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
    SbObject *other;

    other = SbTuple_GetItem(args, 0);
    if (!other) {
        return NULL;
    }
    return SbInt_Add(self, other);
}

static SbObject *
int_sub(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *other;

    other = SbTuple_GetItem(args, 0);
    if (!other) {
        return NULL;
    }
    return SbInt_Subtract(self, other);
}

static SbObject *
int_mul(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *other;

    other = SbTuple_GetItem(args, 0);
    if (!other) {
        return NULL;
    }
    return SbInt_Multiply(self, other);
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
int_and(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return NULL;
}

static SbObject *
int_or(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return NULL;
}

static SbObject *
int_xor(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return NULL;
}

static SbObject *
int_neg(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbInt_Negate(self);
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
    return SbInt_Absolute(self);
}

static SbObject *
int_inv(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbInt_Invert(self);
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
    { "__and__", int_and },
    { "__iand__", int_and },
    { "__or__", int_or },
    { "__ior__", int_or },
    { "__xor__", int_xor },
    { "__ixor__", int_xor },

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
    tp->tp_destroy = (SbDestroyFunc)int_destroy;
    SbInt_Type = tp;
    return 0;
}
