#include "snakebed.h"
#include "internal.h"

#if SUPPORTS(TRACEBACKS)

/* Keep the type object here. */
SbTypeObject *SbTraceBack_Type = NULL;

extern SbFrameObject *SbInterp_TopFrame;

/*
 * C interface implementations
 */

SbObject *
SbTraceBack_FromHere()
{
    SbTraceBackObject *head;
    SbTraceBackObject *prev;
    SbFrameObject *frame;

    /* Avoid trying to build tracebacks when traceback type is not yet there */
    if (!SbTraceBack_Type) {
        Sb_RETURN_NONE;
    }

    frame = SbInterp_TopFrame;
    if (!frame) {
        Sb_RETURN_NONE;
    }

    prev = head = NULL;
    while (frame) {
        SbTraceBackObject *current;

        current = (SbTraceBackObject *)SbObject_New(SbTraceBack_Type);
        if (!current) {
            /* Ugh, OOM? */
            Sb_DECREF(head);
            return NULL;
        }
        if (!head) {
            head = current;
        }

        Sb_INCREF(frame);
        current->frame = frame;
        current->ip = frame->ip - SbStr_AsStringUnsafe(frame->code->code);
        prev->next = current;
        prev = current;
        frame = frame->prev;
    }

    return (SbObject *)head;
}

static void
traceback_destroy(SbTraceBackObject *self)
{
    Sb_CLEAR(self->frame);
    /* NOTE: This can recurse heavily */
    Sb_CLEAR(self->next);
    SbObject_DefaultDestroy((SbObject *)self);
}


int
_Sb_TypeInit_TraceBack()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("<traceback>", NULL, NULL, sizeof(SbTraceBackObject));
    if (!tp) {
        return -1;
    }

    tp->tp_destroy = (SbDestroyFunc)traceback_destroy;

    SbModule_Type = tp;
    return 0;
}

#endif /* SUPPORTS(TRACEBACKS) */
