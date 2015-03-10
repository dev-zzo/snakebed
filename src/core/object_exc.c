#include "snakebed.h"

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

SbObject *_SbErr_MemoryErrorInstance = NULL;


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


static void
exception_destroy(SbExceptionObject *self)
{
    Sb_CLEAR(self->args);
    SbObject_DefaultDestroy((SbObject *)self);
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

/* Type initializer */

static const SbCMethodDef exception_methods[] = {
    { "__getattribute__", (SbCFunction)exception_getattribute },
    /* Sentinel */
    { NULL, NULL },
};

int
_Sb_TypeInit_Exceptions()
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

    _SbErr_MemoryErrorInstance = _SbErr_Instantiate(SbErr_MemoryError, NULL);

    return 0;
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
