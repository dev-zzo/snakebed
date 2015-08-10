#include "snakebed.h"

SbTypeObject *SbErr_Type = NULL;
SbObject *SbErr_Value = NULL;
#if SUPPORTS(TRACEBACKS)
SbTraceBackObject *SbErr_TraceBack = NULL;
#endif

/* Cached MemoryError instance -- when we don't have memory to create another one */
extern SbObject *_SbExc_MemoryErrorInstance;
extern SbObject *_SbExc_StopIterationInstance;

SbTypeObject *
SbErr_Occurred(void)
{
    return SbErr_Type;
}

int
SbExc_ExceptionTypeMatches(SbTypeObject *exc_type, SbObject *what)
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
    Sb_CLEAR(SbErr_Type);
    Sb_CLEAR(SbErr_Value);
#if SUPPORTS(TRACEBACKS)
    Sb_CLEAR(SbErr_TraceBack);
#endif
}

void
SbErr_Fetch(SbTypeObject **type, SbObject **value, SbObject **tb)
{
    *type = SbErr_Type;
    SbErr_Type = NULL;
    *value = SbErr_Value;
    SbErr_Value = NULL;
#if SUPPORTS(TRACEBACKS)
    *tb = (SbObject *)SbErr_TraceBack;
    SbErr_TraceBack = NULL;
#else
    *tb = (SbObject *)NULL;
#endif
}

void
SbErr_Restore(SbTypeObject *type, SbObject *value, SbObject *tb)
{
    SbErr_Clear();
    SbErr_Type = type;
    SbErr_Value = value;
    SbErr_TraceBack = (SbTraceBackObject *)tb;
}


void
SbErr_Raise(SbTypeObject *type, SbObject *value, SbObject *tb)
{
    SbErr_Clear();

    Sb_INCREF(type);
    SbErr_Type = type;

    if (value) {
        if (!SbTuple_CheckExact(value)) {
            if (value != Sb_None) {
                SbObject *packed;

                packed = SbTuple_Pack(1, value);
                /* TODO: How to handle a double fault? */
                Sb_DECREF(value);
                SbErr_Value = packed;
            }
        }
        else {
            Sb_INCREF(value);
            SbErr_Value = value;
        }
    }
    else {
        SbErr_Value = SbTuple_New(0);
    }

#if SUPPORTS(TRACEBACKS)
    if (tb && tb != Sb_None) {
        Sb_INCREF(tb);
        SbErr_TraceBack = (SbTraceBackObject *)tb;
    }
#endif
}

void
SbErr_RaiseWithObject(SbTypeObject *type, SbObject *value)
{
    SbErr_Raise(type, value, NULL);
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
    SbErr_RaiseWithObject(SbExc_IOError, value);
}

static SbObject *
err_raise_singleton(SbTypeObject *which)
{
    SbErr_Clear();
    Sb_INCREF(which);
    SbErr_Restore(which, NULL, NULL);
    return NULL;
}

SbObject *
SbErr_NoMemory(void)
{
    return err_raise_singleton(SbExc_MemoryError);
}

SbObject *
SbErr_NoMoreItems(void)
{
    return err_raise_singleton(SbExc_StopIteration);
}
