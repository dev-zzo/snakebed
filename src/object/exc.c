#include "snakebed.h"
#include "internal.h"

SbTypeObject *SbExc_BaseException = NULL;
SbTypeObject  *SbExc_Exception = NULL;
SbTypeObject   *SbExc_StandardError = NULL;
SbTypeObject    *SbExc_AttributeError = NULL;
SbTypeObject    *SbExc_EnvironmentError = NULL;
SbTypeObject     *SbExc_IOError = NULL;
SbTypeObject    *SbExc_ImportError = NULL;
SbTypeObject    *SbExc_LookupError = NULL;
SbTypeObject     *SbExc_IndexError = NULL;
SbTypeObject     *SbExc_KeyError = NULL;
SbTypeObject    *SbExc_MemoryError = NULL;
SbTypeObject    *SbExc_NameError = NULL;
SbTypeObject     *SbExc_UnboundLocalError = NULL;
SbTypeObject    *SbExc_SystemError = NULL;
SbTypeObject    *SbExc_TypeError = NULL;
SbTypeObject    *SbExc_ValueError = NULL;
SbTypeObject   *SbExc_StopIteration = NULL;
SbTypeObject  *SbExc_SystemExit = NULL;
SbTypeObject  *SbExc_KeyboardInterrupt = NULL;

SbObject *_SbExc_MemoryErrorInstance = NULL;
SbObject *_SbExc_StopIterationInstance = NULL;

int
SbExc_Check(SbObject *o)
{
    return SbType_IsSubtype(Sb_TYPE(o), SbExc_BaseException);
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
    if (!SbRT_StrCmp(attr_str, "args")) {
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
    SbObject *result;

    if (SbArgs_Parse("S:name", args, kwargs, &attr_name) < 0) {
        return NULL;
    }
    result = exception_getattr_internal(self, attr_name);
    if (result) {
        return result;
    }
    return SbObject_DefaultGetAttr((SbObject *)self, args, kwargs);
}

static SbObject *
exception_str(SbBaseExceptionObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *arg_repr;
    Sb_ssize_t arg_count;

    arg_count = self->args ? SbTuple_GetSizeUnsafe(self->args) : 0;
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
    { "__setattr__", SbObject_DefaultSetAttr },
    { "__delattr__", SbObject_DefaultDelAttr },
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
    if (!SbRT_StrCmp(attr_str, "errno") && args_count > 1) {
        value = SbTuple_GetItem(args, 0);
        goto do_return;
    }
    if (!SbRT_StrCmp(attr_str, "strerror") && args_count > 1) {
        value = SbTuple_GetItem(args, 1);
        goto do_return;
    }
    if (!SbRT_StrCmp(attr_str, "filename")) {
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
    SbObject *result;

    if (SbArgs_Parse("S:name", args, kwargs, &attr_name) < 0) {
        return NULL;
    }
    result = enverror_getattr_internal(self, attr_name);
    if (result) {
        return result;
    }
    return SbObject_DefaultGetAttr((SbObject *)self, args, kwargs);
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
    SbExc_BaseException = tp;

    SbExc_Exception = SbExc_NewException("Exception", SbExc_BaseException);
    SbExc_StandardError = SbExc_NewException("StandardError", SbExc_Exception);

    SbExc_AttributeError = SbExc_NewException("AttributeError", SbExc_StandardError);

    tp = _SbType_FromCDefs("EnvironmentError", SbExc_StandardError, enverror_methods, sizeof(SbBaseExceptionObject));
    tp->tp_destroy = (SbDestroyFunc)exception_destroy;
    SbExc_EnvironmentError = tp;

    SbExc_ImportError = SbExc_NewException("ImportError", SbExc_StandardError);
    SbExc_LookupError = SbExc_NewException("LookupError", SbExc_StandardError);
    SbExc_MemoryError = SbExc_NewException("MemoryError", SbExc_StandardError);
    SbExc_NameError = SbExc_NewException("NameError", SbExc_StandardError);
    SbExc_SystemError = SbExc_NewException("SystemError", SbExc_StandardError);
    SbExc_TypeError = SbExc_NewException("TypeError", SbExc_StandardError);
    SbExc_ValueError = SbExc_NewException("ValueError", SbExc_StandardError);

    SbExc_IOError = SbExc_NewException("IOError", SbExc_EnvironmentError);

    SbExc_IndexError = SbExc_NewException("IndexError", SbExc_LookupError);
    SbExc_KeyError = SbExc_NewException("KeyError", SbExc_LookupError);

    SbExc_UnboundLocalError = SbExc_NewException("UnboundLocalError", SbExc_NameError);

    SbExc_StopIteration = SbExc_NewException("StopIteration", SbExc_Exception);
    SbExc_SystemExit = SbExc_NewException("SystemExit", SbExc_BaseException);
    SbExc_KeyboardInterrupt = SbExc_NewException("KeyboardInterrupt", SbExc_BaseException);

    _SbExc_MemoryErrorInstance = SbObject_Call((SbObject *)SbExc_MemoryError, NULL, NULL);
    _SbExc_StopIterationInstance = SbObject_Call((SbObject *)SbExc_StopIteration, NULL, NULL);

    return 0;
}


SbTypeObject *
SbExc_NewException(const char *name, SbTypeObject *base)
{
    SbTypeObject *tp;

    tp = SbType_New(name, base, NULL);
    if (!tp) {
        return NULL;
    }
    return tp;
}
