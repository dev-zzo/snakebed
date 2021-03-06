#include "snakebed.h"

SbInt_Native_t
SbObject_Hash(SbObject *p)
{
    SbObject *result;
    SbInt_Native_t hash;

    result = SbObject_CallMethod(p, "__hash__", NULL, NULL);
    if (result) {
        hash = SbInt_AsNative(result);
        Sb_DECREF(result);
    }
    else {
        hash = -1;
    }
    return hash;
}

int
SbObject_IsTrue(SbObject *p)
{
    SbObject *result;

    /* Shortcuts */
    if (p == Sb_None || p == Sb_False) {
        return 0;
    }
    if (p == Sb_True) {
        return 1;
    }

    result = SbObject_CallMethod(p, "__nonzero__", NULL, NULL);
    if (!result && SbExc_ExceptionTypeMatches(SbErr_Occurred(), (SbObject *)SbExc_AttributeError)) {
        SbErr_Clear();
        result = SbObject_CallMethod(p, "__len__", NULL, NULL);
        if (!result && SbExc_ExceptionTypeMatches(SbErr_Occurred(), (SbObject *)SbExc_AttributeError)) {
            SbErr_Clear();
            return 1;
        }
    }

    if (!result) {
        return -1;
    }

    if (SbBool_Check(result)) {
        int val;

        val = result == Sb_True;
        Sb_DECREF(result);
        return val;
    }
    if (SbInt_CheckExact(result)) {
        long val;

        val = SbInt_AsNative(result);
        Sb_DECREF(result);
        return val != 0;
    }

    Sb_DECREF(result);
    return -1;
}

int
SbObject_Not(SbObject *p)
{
    int is_true;

    is_true = SbObject_IsTrue(p);
    if (is_true == -1) {
        return is_true;
    }

    return is_true == 0;
}

SbObject *
SbObject_Str(SbObject *p)
{
    SbObject *result;

    result = SbObject_CallMethod(p, "__str__", NULL, NULL);
    if (!result && SbExc_ExceptionTypeMatches(SbErr_Occurred(), (SbObject *)SbExc_AttributeError)) {
        SbErr_Clear();
        result = SbObject_Repr(p);
    }

    return result;
}

SbObject *
SbObject_Repr(SbObject *p)
{
    SbObject *result;

    result = SbObject_CallMethod(p, "__repr__", NULL, NULL);
    if (!result && SbExc_ExceptionTypeMatches(SbErr_Occurred(), (SbObject *)SbExc_AttributeError)) {
        SbErr_Clear();
        result = SbObject_DefaultStr(p, NULL, NULL);
    }

    return result;
}

static const char *op_to_method[] = {
    "__lt__",
    "__le__",
    "__eq__",
    "__ne__",
    "__gt__",
    "__ge__",
};

SbObject *
SbObject_Compare(SbObject *p1, SbObject *p2, SbObjectCompareOp op)
{
    SbObject *result;

    /* No need to look up things for these */
    if (p1 == p2) {
        if (op == Sb_EQ) {
            Sb_RETURN_TRUE;
        }
        if (op == Sb_NE) {
            Sb_RETURN_FALSE;
        }
    }

    result = SbObject_CallMethodObjArgs(p1, op_to_method[op], 1, p2);
    if (!result && SbExc_ExceptionTypeMatches(SbErr_Occurred(), (SbObject *)SbExc_AttributeError)) {
        Sb_INCREF(Sb_NotImplemented);
        return Sb_NotImplemented;
    }
    return result;
}

int
SbObject_CompareBool(SbObject *p1, SbObject *p2, SbObjectCompareOp op)
{
    SbObject *result;

    /* No need to look up things for these */
    if (p1 == p2) {
        if (op == Sb_EQ)
            return 1;
        if (op == Sb_NE)
            return 0;
    }

    result = SbObject_CallMethodObjArgs(p1, op_to_method[op], 1, p2);
    if (!result) {
        return -1;
    }
    if (result == Sb_NotImplemented) {
        Sb_DECREF(result);
        return -1;
    }

    return SbObject_IsTrue(result);
}

/* Lookup a method within the type dictionary.
   Returns: New reference. */
static SbObject *
type_method_check(SbObject *p, const char *method_name)
{
    SbObject *attr;
    SbTypeObject *tp;

    tp = Sb_TYPE(p);
    attr = SbDict_GetItemString(tp->tp_dict, method_name);
    if (attr) {
        if (SbCFunction_Check(attr) || SbPFunction_Check(attr)) {
            return SbMethod_New(tp, attr, p);
        }
    }
    return NULL;
}

SbObject *
SbObject_GetAttrString(SbObject *p, const char *attr_name)
{
    SbObject *getattribute;
    SbObject *getattr;
    SbObject *attr;

    /* https://docs.python.org/2/reference/datamodel.html#more-attribute-access-for-new-style-classes */
    getattribute = type_method_check(p, "__getattribute__");
    if (getattribute) {
        attr = SbObject_CallObjArgs(getattribute, 1, SbStr_FromString(attr_name));
        Sb_DECREF(getattribute);
        /* Silence AttributeError, if any */
        if (!attr && SbExc_ExceptionTypeMatches(SbErr_Occurred(), (SbObject *)SbExc_AttributeError)) {
            SbErr_Clear();
        }
        return attr;
    }

    /* For class object lookups, produce an unbound method object */
    if (Sb_TYPE(p) == SbType_Type) {
        attr = SbDict_GetItemString(SbObject_DICT(p), attr_name);
        if (attr) {
            if (SbCFunction_Check(attr) || SbPFunction_Check(attr)) {
                attr = SbMethod_New((SbTypeObject *)p, attr, NULL);
                return attr;
            }
        }
    }

    /* Note: inlined type_method_check to avoid double lookup */
    attr = SbDict_GetItemString(Sb_TYPE(p)->tp_dict, attr_name);
    if (attr) {
        if (SbCFunction_Check(attr) || SbPFunction_Check(attr)) {
            attr = SbMethod_New(Sb_TYPE(p), attr, p);
        }
        else {
            Sb_INCREF(attr);
        }
        return attr;
    }

    /* https://docs.python.org/2/reference/datamodel.html#customizing-attribute-access */
    /* Note that if the attribute is found through the normal mechanism, 
       __getattr__() is not called. */
    getattr = type_method_check(p, "__getattr__");
    if (getattr) {
        attr = SbObject_CallObjArgs(getattr, 1, SbStr_FromString(attr_name));
        Sb_DECREF(getattr);
        /* Silence AttributeError, if any */
        if (!attr && SbExc_ExceptionTypeMatches(SbErr_Occurred(), (SbObject *)SbExc_AttributeError)) {
            SbErr_Clear();
        }
        return attr;
    }

    /* NOTE: This API does NOT raise exceptions if an attribute is not found. */
    return NULL;
}

int
SbObject_SetAttrString(SbObject *p, const char *attr_name, SbObject *v)
{
    SbObject *setattr;

    /* Look for a descriptor in type hierarchy first? */
    setattr = type_method_check(p, "__setattr__");
    if (setattr) {
        SbObject *result;

        Sb_INCREF(v);
        result = SbObject_CallObjArgs(setattr, 2, SbStr_FromString(attr_name), v);
        Sb_XDECREF(result);
        return result ? 0 : -1;
    }

    SbErr_RaiseWithString(SbExc_AttributeError, attr_name);
    return -1;
}

int
SbObject_DelAttrString(SbObject *p, const char *attr_name)
{
    SbObject *delattr;

    /* Look for a descriptor in type hierarchy first? */
    delattr = type_method_check(p, "__delattr__");
    if (delattr) {
        SbObject *result;

        result = SbObject_CallObjArgs(delattr, 1, SbStr_FromString(attr_name));
        Sb_XDECREF(result);
        return result ? 0 : -1;
    }

    SbErr_RaiseWithString(SbExc_AttributeError, attr_name);
    return -1;
}

/* Callable interface */

SbObject *
SbObject_Call(SbObject *callable, SbObject *args, SbObject *kwargs)
{
    SbObject *m_call;

    /* Avoid recursion. */
    if (SbCFunction_Check(callable)) {
        return SbCFunction_Call(callable, NULL, args, kwargs);
    }
    if (SbPFunction_Check(callable)) {
        return SbPFunction_Call(callable, args, kwargs);
    }
    if (SbMethod_Check(callable)) {
        return SbMethod_Call(callable, args, kwargs);
    }
    m_call = type_method_check(callable, "__call__");
    if (m_call) {
        SbObject *result;
        result = SbObject_Call(m_call, args, kwargs);
        Sb_DECREF(m_call);
        return result;
    }

    SbErr_RaiseWithFormat(SbExc_AttributeError, "the `%s` instance has no `%s` method", Sb_TYPE(callable)->tp_name, "__call__");
    return NULL;
}

SbObject *
SbObject_CallObjArgs(SbObject *callable, Sb_ssize_t count, ...)
{
    SbObject *args;
    SbObject *result;
    va_list va;

    va_start(va, count);
    args = SbTuple_PackVa(count, va);
    va_end(va);
    if (!args) {
        return NULL;
    }

    result = SbObject_Call(callable, args, NULL);

    Sb_DECREF(args);
    return result;
}

SbObject *
SbObject_CallMethod(SbObject *o, const char *method, SbObject *args, SbObject *kwargs)
{
    SbObject *m;
    SbObject *result;

    /* m = SbObject_GetAttrString(o, method); */
    m = type_method_check(o, method);
    if (!m) {
        SbErr_RaiseWithString(SbExc_AttributeError, method);
        return NULL;
    }

    result = SbObject_Call(m, args, kwargs);
    Sb_DECREF(m);
    return result;
}

SbObject *
SbObject_CallMethodObjArgs(SbObject *o, const char *method, Sb_ssize_t count, ...)
{
    SbObject *args;
    SbObject *result;
    va_list va;

    va_start(va, count);
    args = SbTuple_PackVa(count, va);
    va_end(va);
    if (!args) {
        return NULL;
    }

    result = SbObject_CallMethod(o, method, args, NULL);

    Sb_DECREF(args);
    return result;
}

