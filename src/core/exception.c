#include "snakebed.h"

typedef struct _SbExceptionObject {
    SbObject_HEAD;
    SbObject *args;
} SbExceptionObject;

SbTypeObject *SbErr_Exception = NULL;
SbTypeObject  *SbErr_StandardError = NULL;
SbTypeObject   *SbErr_AttributeError = NULL;
SbTypeObject   *SbErr_LookupError = NULL;
SbTypeObject    *SbErr_IndexError = NULL;
SbTypeObject    *SbErr_KeyError = NULL;
SbTypeObject   *SbErr_MemoryError = NULL;
SbTypeObject   *SbErr_NameError = NULL;
SbTypeObject    *SbErr_UnboundLocalError = NULL;
SbTypeObject   *SbErr_SystemError = NULL;
SbTypeObject   *SbErr_TypeError = NULL;
SbTypeObject   *SbErr_ValueError = NULL;

static SbExceptionInfo exception = { NULL, NULL, NULL, };

/* Cached MemoryError instance -- when we don't have memory to create another one */
static SbObject *_memory_error_instance = NULL;

SbTypeObject *
SbErr_Occurred(void)
{
    return exception.type;
}

int
SbErr_ExceptionMatches(SbTypeObject *exc, SbObject *what)
{
    if (SbType_Check(what) && SbType_IsSubtype(exc, (SbTypeObject *)what)) {
        return 1;
    }

    if (SbTuple_CheckExact(what)) {
        Sb_ssize_t pos, count;

        count = SbTuple_GetSizeUnsafe(what);
        for (pos = 0; pos < count; ++pos) {
            SbTypeObject *tp;

            tp = (SbTypeObject *)SbTuple_GetItemUnsafe(what, pos);
            if (SbType_IsSubtype(exc, tp)) {
                return 1;
            }
        }
    }

    return 0;
}

void
SbErr_Clear(void)
{
    Sb_CLEAR(exception.type);
    Sb_CLEAR(exception.value);
    Sb_CLEAR(exception.traceback);
}

void
SbErr_Fetch(SbExceptionInfo *info)
{
    *info = exception;
    exception.type = NULL;
    exception.value = NULL;
    exception.traceback = NULL;
}

void
SbErr_FetchCopy(SbExceptionInfo *info)
{
    *info = exception;
    if (info->type)
        Sb_INCREF(info->type);
    if (info->value)
        Sb_INCREF(info->value);
    if (info->traceback)
        Sb_INCREF(info->traceback);
}

void
SbErr_Restore(SbExceptionInfo *info)
{
    SbErr_Clear();
    exception = *info;
}

SbObject *
_SbErr_Instantiate(SbTypeObject *type, SbObject *value)
{
    SbExceptionObject *e;
    SbObject *args;

    e = (SbExceptionObject *)SbObject_New(type);
    if (!e) {
        /* Whoopsie... */
        return SbErr_NoMemory();
    }

    if (value) {
        Sb_INCREF(value);
        args = SbTuple_Pack(1, value);
    }
    else {
        args = SbTuple_New(0);
    }

    if (!args) {
        Sb_DECREF(e);
        return SbErr_NoMemory();
    }
    e->args = args;

    return (SbObject *)e;
}


void
SbErr_RaiseWithObject(SbTypeObject *type, SbObject *value)
{
    SbObject *e;

    SbErr_Clear();

    e = _SbErr_Instantiate(type, value);
    if (!e) {
        return;
    }

    Sb_INCREF(type);
    exception.type = type;
    exception.value = e;
}

void
SbErr_RaiseWithString(SbTypeObject *type, const char *value)
{
    SbObject *s;

    s = SbStr_FromString(value);
    if (!s) {
        /* TODO: How to handle a double fault? */
    }
    SbErr_RaiseWithObject(type, s);
    Sb_DECREF(s);
}

void
SbErr_RaiseWithFormat(SbTypeObject *type, const char *format, ...)
{
    SbErr_RaiseWithString(type, "{SbErr_RaiseWithFormat not properly implemented yet}");
}

SbObject *
SbErr_NoMemory(void)
{
    /* This deserves special handling. */
    SbErr_Clear();
    Sb_INCREF(SbErr_MemoryError);
    exception.type = SbErr_MemoryError;
    Sb_INCREF(_memory_error_instance);
    exception.value = _memory_error_instance;
    return NULL;
}


static void
exception_destroy(SbExceptionObject *self)
{
    Sb_CLEAR(self->args);
    SbObject_DefaultDestroy((SbObject *)self);
}

SbTypeObject *
SbErr_NewException(const char *name, SbTypeObject *base)
{
    SbTypeObject *tp;

    tp = SbType_New(name, base);
    if (!tp) {
        return NULL;
    }
    tp->tp_basicsize = sizeof(SbExceptionObject);
    tp->tp_destroy = (SbDestroyFunc)exception_destroy;
    return tp;
}

static SbObject *
exception_getattribute(SbExceptionObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *attr_name;
    const char *attr_str;
    SbObject *value;

    if (SbTuple_Unpack(args, 1, 1, &attr_name) < 0) {
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

static const SbCMethodDef exception_methods[] = {
    { "__getattribute__", (SbCFunction)exception_getattribute },
    /* Sentinel */
    { NULL, NULL },
};

int
_SbErr_BuiltinInit()
{
    SbErr_Exception = SbErr_NewException("Exception", NULL);
    SbType_CreateMethods(SbErr_Exception, exception_methods);

    SbErr_StandardError = SbErr_NewException("StandardError", SbErr_Exception);

    SbErr_AttributeError = SbErr_NewException("AttributeError", SbErr_StandardError);
    SbErr_LookupError = SbErr_NewException("LookupError", SbErr_StandardError);
    SbErr_MemoryError = SbErr_NewException("MemoryError", SbErr_StandardError);
    SbErr_NameError = SbErr_NewException("NameError", SbErr_StandardError);
    SbErr_SystemError = SbErr_NewException("SystemError", SbErr_StandardError);
    SbErr_TypeError = SbErr_NewException("TypeError", SbErr_StandardError);
    SbErr_ValueError = SbErr_NewException("ValueError", SbErr_StandardError);

    SbErr_IndexError = SbErr_NewException("IndexError", SbErr_LookupError);
    SbErr_KeyError = SbErr_NewException("KeyError", SbErr_LookupError);

    SbErr_UnboundLocalError = SbErr_NewException("UnboundLocalError", SbErr_NameError);

    _memory_error_instance = _SbErr_Instantiate(SbErr_MemoryError, NULL);

    return 0;
}
