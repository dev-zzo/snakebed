#include "snakebed.h"

SbObject *SbErr_Current = NULL;

/* Cached MemoryError instance -- when we don't have memory to create another one */
extern SbObject *_SbErr_MemoryErrorInstance;
extern SbObject *_SbErr_StopIterationInstance;

SbObject *
SbErr_Occurred(void)
{
    return SbErr_Current;
}

int
SbErr_ExceptionMatches(SbObject *exc, SbObject *what)
{
    if (!exc) {
        return 0;
    }

    return SbErr_ExceptionTypeMatches(Sb_TYPE(exc), what);
}

int
SbErr_ExceptionTypeMatches(SbTypeObject *exc_type, SbObject *what)
{
    if (SbType_Check(what)) {
        if (SbType_IsSubtype(exc_type, (SbTypeObject *)what)) {
            return 1;
        }
    }
    else if (SbTuple_CheckExact(what)) {
        Sb_ssize_t pos, count;

        count = SbTuple_GetSizeUnsafe(what);
        for (pos = 0; pos < count; ++pos) {
            SbTypeObject *tp;

            tp = (SbTypeObject *)SbTuple_GetItemUnsafe(what, pos);
            /* assert(SbType_Check(tp)); */
            if (SbType_IsSubtype(exc_type, tp)) {
                return 1;
            }
        }
    }

    return 0;
}

void
SbErr_Clear(void)
{
    Sb_CLEAR(SbErr_Current);
}

void
SbErr_Fetch(SbObject **exc)
{
    *exc = SbErr_Current;
    SbErr_Current = NULL;
}

void
SbErr_Restore(SbObject *exc)
{
    SbErr_Clear();
    SbErr_Current = exc;
}


void
SbErr_RaiseWithObject(SbTypeObject *type, SbObject *value)
{
    SbObject *args = NULL;

    SbErr_Clear();

    if (value) {
        if (SbTuple_CheckExact(value)) {
            args = value;
        }
        else {
            args = SbTuple_Pack(1, value);
            Sb_DECREF(value);
        }
    }
    SbErr_Current = SbObject_Call((SbObject *)type, args, NULL);
    Sb_XDECREF(args);
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
}

void
SbErr_RaiseWithFormat(SbTypeObject *type, const char *format, ...)
{
    va_list va;
    SbObject *s;

    va_start(va, format);
    s = SbStr_FromFormatVa(format, va);
    va_end(va);
    if (!s) {
        /* TODO: How to handle a double fault? */
    }

    SbErr_RaiseWithObject(type, s);
}

void
SbErr_RaiseIOError(SbInt_Native_t error_code, const char *error_text)
{
    SbObject *value;
    SbObject *o_errno;
    SbObject *o_strerror;

    o_errno = SbInt_FromNative(error_code);
    if (!error_text) {
        error_text = Sb_StrError(error_code);
        if (!error_text) {
            error_text = "no description available";
        }
    }
    o_strerror = SbStr_FromString(error_text);
    value = SbTuple_Pack(2, o_errno, o_strerror);
    Sb_DECREF(o_errno);
    Sb_DECREF(o_strerror);
    SbErr_RaiseWithObject(SbErr_IOError, value);
}

static SbObject *
err_raise_singleton(SbObject *which)
{
    SbErr_Clear();
    Sb_INCREF(which);
    SbErr_Current = which;
    return NULL;
}

SbObject *
SbErr_NoMemory(void)
{
    return err_raise_singleton(_SbErr_MemoryErrorInstance);
}

SbObject *
SbErr_NoMoreItems(void)
{
    return err_raise_singleton(_SbErr_StopIterationInstance);
}
