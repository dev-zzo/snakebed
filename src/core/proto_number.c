#include "snakebed.h"

static SbObject *
numeric_try_method2(SbObject * lhs, SbObject *rhs, const char *method)
{
    SbObject *result;
    SbObject *tmp;

    Sb_INCREF(rhs);
    tmp = SbTuple_Pack(1, rhs);
    result = SbObject_CallMethod(lhs, method, tmp, NULL);
    Sb_DECREF(tmp);

    if (result) {
        return result;
    }

    if (SbErr_ExceptionMatches(SbErr_Occurred(), (SbObject *)SbErr_AttributeError)) {
        SbErr_Clear();
        result = Sb_NotImplemented;
        Sb_INCREF(result);
    }
    return result;
}

static SbObject *
numeric_try_methods2(SbObject * lhs, SbObject *rhs, const char *method, char *rmethod)
{
    SbObject *result;

    /* Try lhs.__op__(rhs) */
    result = numeric_try_method2(lhs, rhs, method);
    if (result != Sb_NotImplemented) {
        return result;
    }
    Sb_DECREF(result);

    /* Try rhs.__rop__(lhs) */
    result = numeric_try_method2(rhs, lhs, rmethod);
    if (result != Sb_NotImplemented) {
        return result;
    }
    Sb_DECREF(result);
    SbErr_RaiseWithString(SbErr_TypeError, "unsupported operand type");
    return NULL;
}

SbObject *
SbNumber_Add(SbObject * lhs, SbObject *rhs)
{
    return numeric_try_methods2(lhs, rhs, "__add__", "__radd__");
}

SbObject *
SbNumber_Subtract(SbObject * lhs, SbObject *rhs)
{
    return numeric_try_methods2(lhs, rhs, "__sub__", "__rsub__");
}

SbObject *
SbNumber_Multiply(SbObject * lhs, SbObject *rhs)
{
    return numeric_try_methods2(lhs, rhs, "__mul__", "__rmul__");
}

SbObject *
SbNumber_Divide(SbObject * lhs, SbObject *rhs)
{
    return numeric_try_methods2(lhs, rhs, "__div__", "__rdiv__");
}

SbObject *
SbNumber_FloorDivide(SbObject * lhs, SbObject *rhs)
{
    return numeric_try_methods2(lhs, rhs, "__floordiv__", "__rfloordiv__");
}

SbObject *
SbNumber_TrueDivide(SbObject * lhs, SbObject *rhs)
{
    return numeric_try_methods2(lhs, rhs, "__truediv__", "__rtruediv__");
}

SbObject *
SbNumber_Remainder(SbObject * lhs, SbObject *rhs)
{
    return numeric_try_methods2(lhs, rhs, "__mod__", "__rmod__");
}

SbObject *
SbNumber_And(SbObject * lhs, SbObject *rhs)
{
    return numeric_try_methods2(lhs, rhs, "__and__", "__rand__");
}

SbObject *
SbNumber_Or(SbObject * lhs, SbObject *rhs)
{
    return numeric_try_methods2(lhs, rhs, "__xor__", "__rxor__");
}

SbObject *
SbNumber_Xor(SbObject * lhs, SbObject *rhs)
{
    return numeric_try_methods2(lhs, rhs, "__or__", "__ror__");
}

SbObject *
SbNumber_Lshift(SbObject * lhs, SbObject *rhs)
{
    return numeric_try_methods2(lhs, rhs, "__lshift__", "__rlshift__");
}

SbObject *
SbNumber_Rshift(SbObject * lhs, SbObject *rhs)
{
    return numeric_try_methods2(lhs, rhs, "__rshift__", "__rrshift__");
}

static SbObject *
numeric_try_method1(SbObject *rhs, const char *method)
{
    SbObject *result;

    result = SbObject_CallMethod(rhs, method, NULL, NULL);
    if (result == Sb_NotImplemented) {
        Sb_DECREF(result);
        SbErr_RaiseWithString(SbErr_TypeError, "unsupported operand type");
        return NULL;
    }

    if (result) {
        return result;
    }

    if (SbErr_ExceptionMatches(SbErr_Occurred(), (SbObject *)SbErr_AttributeError)) {
        SbErr_Clear();
        SbErr_RaiseWithString(SbErr_TypeError, "unsupported operand type");
    }
    return result;
}

SbObject *
SbNumber_Negative(SbObject * rhs)
{
    return numeric_try_method1(rhs, "__neg__");
}

SbObject *
SbNumber_Positive(SbObject * rhs)
{
    return numeric_try_method1(rhs, "__pos__");
}

SbObject *
SbNumber_Absolute(SbObject * rhs)
{
    return numeric_try_method1(rhs, "__abs__");
}

SbObject *
SbNumber_Invert(SbObject * rhs)
{
    return numeric_try_method1(rhs, "__invert__");
}

