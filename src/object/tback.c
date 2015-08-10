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

        Sb_INCREF(frame);
        current->frame = frame;
        current->ip = frame->ip - SbStr_AsStringUnsafe(frame->code->code);
        if (!head) {
            head = current;
        }
        else {
            prev->next = current;
        }
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

SbObject *
SbTraceBack_FormatTrace(SbTraceBackObject *tb)
{
    return SbStr_FromFormat("  in %s +%d\n", SbStr_AsStringUnsafe(tb->frame->code->name), tb->ip);
}

SbObject *
SbTraceBack_FormatException(SbTypeObject *type, SbObject *value)
{
    SbObject *exc_instance;
    SbObject *text;

    exc_instance = SbObject_Call((SbObject *)type, value, NULL);
    if (!exc_instance) {
        return NULL;
    }
    text = SbObject_Str(exc_instance);
    Sb_DECREF(exc_instance);
    return text;
}

static int
append_line(SbObject *list, SbObject *line)
{
    int rv;

    if (!line)
        return -1;
    rv = SbList_Append(list, line);
    Sb_DECREF(line);
    return 0;
}

int
SbTraceBack_PrintException(SbTypeObject *type, SbObject *value, SbObject *tb, Sb_ssize_t limit, SbObject *file)
{
    int rv;
    SbObject *lines;
    Sb_ssize_t line_count;
    Sb_ssize_t line_index;

    rv = -1;

    lines = SbList_New(0);
    if (!lines) {
        goto exit0;
    }

    if (tb && tb != Sb_None) {
        SbTraceBackObject *real_tb = (SbTraceBackObject *)tb;

        if (append_line(lines, SbStr_FromString("Traceback (most recent call last):\n")) < 0) {
            goto exit1;
        }

        while (real_tb && limit-- > 0) {
            if (append_line(lines, SbTraceBack_FormatTrace(real_tb)) < 0) {
                goto exit1;
            }
            real_tb = real_tb->next;
        }
    }

    if (append_line(lines, SbTraceBack_FormatException(type, value)) < 0) {
        goto exit1;
    }

    line_count = SbList_GetSizeUnsafe(lines);
    for (line_index = 0; line_index < line_count; ++line_index) {
        SbFile_WriteString(file, SbStr_AsStringUnsafe(SbList_GetItemUnsafe(lines, line_index)));
    }

    rv = 0;

exit1:
    Sb_DECREF(lines);

exit0:
    return rv;
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

    SbTraceBack_Type = tp;
    return 0;
}

#endif /* SUPPORTS(TRACEBACKS) */
