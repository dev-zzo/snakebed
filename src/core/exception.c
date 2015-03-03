#include "snakebed.h"

static SbExceptionInfo exception = { NULL, NULL, NULL, };

SbTypeObject *
SbErr_Occurred(void)
{
    return exception.type;
}

int
SbErr_ExceptionMatches(SbTypeObject *exc, SbObject *what)
{
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
SbErr_RaiseWithObject(SbTypeObject *type, SbObject *value)
{
    SbErr_Clear();
    Sb_INCREF(type);
    exception.type = type;
    Sb_INCREF(value);
    exception.value = value;
}

void
SbErr_RaiseWithString(SbTypeObject *type, const char *value)
{
    SbErr_RaiseWithObject(type, SbStr_FromString(value));
}

void
SbErr_RaiseWithFormat(SbTypeObject *type, const char *format, ...)
{
    SbErr_RaiseWithString(type, "{SbErr_RaiseWithFormat not properly implemented yet}");
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
SbErr_Restore(SbExceptionInfo *info)
{
    SbErr_Clear();
    exception = *info;
}


SbObject *
SbErr_NoMemory(void)
{
    SbErr_RaiseWithObject(SbErr_MemoryError, Sb_None);
    return NULL;
}


/* Dummy object -- these are never instantiated. */
typedef struct _SbExceptionObject {
    SbObject_HEAD;
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

SbTypeObject *
SbErr_NewException(const char *name, SbTypeObject *base)
{
    SbTypeObject *tp;

    tp = SbType_New(name, base);
    if (!tp) {
        return NULL;
    }
    tp->tp_basicsize = sizeof(SbExceptionObject);
    tp->tp_destroy = SbObject_DefaultDestroy;
    return tp;
}


int
_SbErr_BuiltinInit()
{
    SbErr_Exception = SbErr_NewException("Exception", NULL);
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

    return 0;
}
