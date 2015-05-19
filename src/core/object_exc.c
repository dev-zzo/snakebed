#include "snakebed.h"

SbTypeObject *SbErr_BaseException = NULL;
SbTypeObject  *SbErr_Exception = NULL;
SbTypeObject   *SbErr_StandardError = NULL;
SbTypeObject    *SbErr_AttributeError = NULL;
SbTypeObject    *SbErr_EnvironmentError = NULL;
SbTypeObject     *SbErr_IOError = NULL;
SbTypeObject    *SbErr_ImportError = NULL;
SbTypeObject    *SbErr_LookupError = NULL;
SbTypeObject     *SbErr_IndexError = NULL;
SbTypeObject     *SbErr_KeyError = NULL;
SbTypeObject    *SbErr_MemoryError = NULL;
SbTypeObject    *SbErr_NameError = NULL;
SbTypeObject     *SbErr_UnboundLocalError = NULL;
SbTypeObject    *SbErr_SystemError = NULL;
SbTypeObject    *SbErr_TypeError = NULL;
SbTypeObject    *SbErr_ValueError = NULL;
SbTypeObject   *SbErr_StopIteration = NULL;
SbTypeObject  *SbErr_SystemExit = NULL;

SbObject *_SbErr_MemoryErrorInstance = NULL;
SbObject *_SbErr_StopIterationInstance = NULL;

int
SbErr_Check(SbObject *o)
{
    return SbType_IsSubtype(Sb_TYPE(o), SbErr_BaseException);
}

static void
exception_destroy(SbBaseExceptionObject *self)
{
    Sb_CLEAR(self->args);
    SbObject_DefaultDestroy((SbObject *)self);
}

static SbObject *
exception_init(SbBaseExceptionObject *self, SbObject *args, SbObject *kwargs)
{
    if (args) {
        Sb_INCREF(args);
        self->args = args;
    }
    Sb_RETURN_NONE;
}

static SbObject *
exception_getattr_internal(SbBaseExceptionObject *self, SbObject *attr_name)
{
    const char *attr_str;
    SbObject *value;

    attr_str = SbStr_AsStringUnsafe(attr_name);
    if (!Sb_StrCmp(attr_str, "args")) {
        value = self->args;
        Sb_INCREF(value);
        return value;
    }
    return NULL;
}

static SbObject *
exception_getattr(SbBaseExceptionObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *attr_name;

    if (SbArgs_Unpack(args, 1, 1, &attr_name) < 0) {
        return NULL;
    }
    if (!SbStr_CheckExact(attr_name)) {
        SbErr_RaiseWithString(SbErr_TypeError, "attribute name must be a string");
        return NULL;
    }
    return exception_getattr_internal(self, attr_name);
}

static SbObject *
exception_str(SbBaseExceptionObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *arg_repr;
    Sb_ssize_t arg_count;

    arg_count = SbTuple_GetSizeUnsafe(self->args);
    if (arg_count == 0) {
        return SbStr_FromFormat("%s: (no message)", Sb_TYPE(self)->tp_name);
    }
    if (arg_count == 1) {
        arg_repr = SbTuple_GetItemUnsafe(self->args, 0);
        if (SbStr_CheckExact(arg_repr)) {
            return SbStr_FromFormat("%s: %s", Sb_TYPE(self)->tp_name, SbStr_AsStringUnsafe(arg_repr));
        }
    }
    arg_repr = SbObject_Str(self->args);
    return SbStr_FromFormat("%s: %s", Sb_TYPE(self)->tp_name, SbStr_AsStringUnsafe(arg_repr));
}

/* Type initializer */

static const SbCMethodDef exception_methods[] = {
    { "__init__", (SbCFunction)exception_init },
    { "__getattr__", (SbCFunction)exception_getattr },
    { "__str__", (SbCFunction)exception_str },
    /* Sentinel */
    { NULL, NULL },
};

static SbObject *
enverror_getattr_internal(SbBaseExceptionObject *self, SbObject *attr_name)
{
    const char *attr_str;
    SbObject *args;
    Sb_ssize_t args_count;
    SbObject *value;

    args = self->args;
    args_count = SbTuple_GetSizeUnsafe(args);
    attr_str = SbStr_AsStringUnsafe(attr_name);
    if (!Sb_StrCmp(attr_str, "errno") && args_count > 1) {
        value = SbTuple_GetItem(args, 0);
        goto do_return;
    }
    if (!Sb_StrCmp(attr_str, "strerror") && args_count > 1) {
        value = SbTuple_GetItem(args, 1);
        goto do_return;
    }
    if (!Sb_StrCmp(attr_str, "filename")) {
        if (args_count == 3) {
            value = SbTuple_GetItem(args, 2);
        }
        else {
            value = Sb_None;
        }
        goto do_return;
    }
    return exception_getattr_internal(self, attr_name);

do_return:
    Sb_INCREF(value);
    return value;
}

static SbObject *
enverror_getattr(SbBaseExceptionObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *attr_name;
    const char *attr_str;
    SbObject *value;

    if (SbArgs_Unpack(args, 1, 1, &attr_name) < 0) {
        return NULL;
    }
    if (!SbStr_CheckExact(attr_name)) {
        SbErr_RaiseWithString(SbErr_TypeError, "attribute name must be a string");
        return NULL;
    }

    attr_str = SbStr_AsStringUnsafe(attr_name);
    if (!Sb_StrCmp(attr_str, "args")) {
        value = self->args;
        Sb_INCREF(value);
        return value;
    }

    return NULL;
}

static SbObject *
enverror_str(SbBaseExceptionObject *self, SbObject *args, SbObject *kwargs)
{
    Sb_ssize_t arg_count;

    arg_count = SbTuple_GetSizeUnsafe(self->args);
    if (arg_count == 3) {
        OSError_t error_code;
        const char *error_text;
        const char *file_name;

        error_code = (OSError_t)SbInt_AsNative(SbTuple_GetItemUnsafe(self->args, 0));
        error_text = (const char *)SbStr_AsString(SbTuple_GetItemUnsafe(self->args, 1));
        file_name = (const char *)SbStr_AsString(SbTuple_GetItemUnsafe(self->args, 2));
        return SbStr_FromFormat("%s: errno %d when accessing %s:\r\n%s", Sb_TYPE(self)->tp_name, error_code, file_name, error_text);
    }
    if (arg_count == 2) {
        OSError_t error_code;
        const char *error_text;

        error_code = (OSError_t)SbInt_AsNative(SbTuple_GetItemUnsafe(self->args, 0));
        error_text = (const char *)SbStr_AsString(SbTuple_GetItemUnsafe(self->args, 1));
        return SbStr_FromFormat("%s: errno %d:\r\n%s", Sb_TYPE(self)->tp_name, error_code, error_text);
    }
    return exception_str(self, args, kwargs);
}

static const SbCMethodDef enverror_methods[] = {
    { "__getattr__", (SbCFunction)enverror_getattr },
    { "__str__", (SbCFunction)enverror_str },
    /* Sentinel */
    { NULL, NULL },
};

int
_Sb_TypeInit_Exceptions()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("BaseException", NULL, exception_methods, sizeof(SbBaseExceptionObject));
    tp->tp_destroy = (SbDestroyFunc)exception_destroy;
    SbErr_BaseException = tp;

    SbErr_Exception = SbErr_NewException("Exception", SbErr_BaseException);
    SbErr_StandardError = SbErr_NewException("StandardError", SbErr_Exception);

    SbErr_AttributeError = SbErr_NewException("AttributeError", SbErr_StandardError);

    tp = _SbType_FromCDefs("EnvironmentError", SbErr_StandardError, enverror_methods, sizeof(SbBaseExceptionObject));
    tp->tp_destroy = (SbDestroyFunc)exception_destroy;
    SbErr_EnvironmentError = tp;

    SbErr_ImportError = SbErr_NewException("ImportError", SbErr_StandardError);
    SbErr_LookupError = SbErr_NewException("LookupError", SbErr_StandardError);
    SbErr_MemoryError = SbErr_NewException("MemoryError", SbErr_StandardError);
    SbErr_NameError = SbErr_NewException("NameError", SbErr_StandardError);
    SbErr_SystemError = SbErr_NewException("SystemError", SbErr_StandardError);
    SbErr_TypeError = SbErr_NewException("TypeError", SbErr_StandardError);
    SbErr_ValueError = SbErr_NewException("ValueError", SbErr_StandardError);

    SbErr_IOError = SbErr_NewException("IOError", SbErr_EnvironmentError);

    SbErr_IndexError = SbErr_NewException("IndexError", SbErr_LookupError);
    SbErr_KeyError = SbErr_NewException("KeyError", SbErr_LookupError);

    SbErr_UnboundLocalError = SbErr_NewException("UnboundLocalError", SbErr_NameError);

    SbErr_StopIteration = SbErr_NewException("StopIteration", SbErr_Exception);

    SbErr_SystemExit = SbErr_NewException("SystemExit", SbErr_BaseException);

    _SbErr_MemoryErrorInstance = SbObject_Call((SbObject *)SbErr_MemoryError, NULL, NULL);
    _SbErr_StopIterationInstance = SbObject_Call((SbObject *)SbErr_StopIteration, NULL, NULL);

    return 0;
}


SbTypeObject *
SbErr_NewException(const char *name, SbTypeObject *base)
{
    SbTypeObject *tp;

    tp = SbType_New(name, base, NULL);
    if (!tp) {
        return NULL;
    }
    return tp;
}
