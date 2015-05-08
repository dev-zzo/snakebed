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


SbObject *
_SbErr_Instantiate(SbTypeObject *type, SbObject *value)
{
    SbBaseExceptionObject *e;
    SbObject *args;

    e = (SbBaseExceptionObject *)SbObject_New(type);
    if (!e) {
        /* Whoopsie... */
        return SbErr_NoMemory();
    }

    if (value) {
        if (SbTuple_CheckExact(value)) {
            args = value;
        }
        else {
            args = SbTuple_Pack(1, value);
            /* Sb_DECREF(value); */
        }
    }
    else {
        args = SbTuple_New(0);
    }

    if (!args) {
        Sb_DECREF(e);
        return NULL;
    }
    e->args = args;

    return (SbObject *)e;
}


static void
exception_destroy(SbBaseExceptionObject *self)
{
    Sb_CLEAR(self->args);
    SbObject_DefaultDestroy((SbObject *)self);
}

static SbObject *
exception_getattribute(SbBaseExceptionObject *self, SbObject *args, SbObject *kwargs)
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

/* Type initializer */

static const SbCMethodDef exception_methods[] = {
    { "__getattribute__", (SbCFunction)exception_getattribute },
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
    SbErr_EnvironmentError = SbErr_NewException("EnvironmentError", SbErr_StandardError);
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

    _SbErr_MemoryErrorInstance = _SbErr_Instantiate(SbErr_MemoryError, NULL);
    _SbErr_StopIterationInstance = _SbErr_Instantiate(SbErr_StopIteration, NULL);

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
