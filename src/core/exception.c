#include "snakebed.h"

static SbExceptionInfo exception = { NULL, NULL, NULL, };

/* Cached MemoryError instance -- when we don't have memory to create another one */
extern SbObject *_SbErr_MemoryErrorInstance;

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
_SbErr_Clear(SbExceptionInfo *info)
{
    Sb_CLEAR(info->type);
    Sb_CLEAR(info->value);
    Sb_CLEAR(info->traceback);
}

void
SbErr_Clear(void)
{
    _SbErr_Clear(&exception);
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
    Sb_INCREF(_SbErr_MemoryErrorInstance);
    exception.value = _SbErr_MemoryErrorInstance;
    return NULL;
}

