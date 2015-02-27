#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbFrame_Type = NULL;

/*
 * C interface implementations
 */

SbObject *
SbFrame_New(SbCodeObject *code, SbObject *globals, SbObject *locals)
{
    SbObject *p;

    p = (SbObject *)SbObject_NewVar(SbFrame_Type, code->stack_size);
    if (p) {
        SbFrameObject *op = (SbFrameObject *)p;

        Sb_INCREF(code);
        op->code = code;

        if (globals) {
            Sb_INCREF(globals);
            op->globals = globals;
        }
        else {
            /* This is interesting. */
        }

        if (locals) {
            Sb_INCREF(locals);
            op->locals = locals;
        }
        else {
            /* TODO: Is this correct? */
            Sb_INCREF(op->globals);
            op->locals = op->globals;
        }

        op->ip = SbStr_AsStringUnsafe(code->code);
        /* stack pointer points just outside the stack */
        op->sp = &op->stack[code->stack_size];
    }
    return p;
}

static void
frame_destroy(SbFrameObject *f)
{
    Sb_CLEAR(f->code);
    Sb_CLEAR(f->globals);
    Sb_CLEAR(f->locals);
    Sb_CLEAR(f->prev);
    SbObject_DefaultDestroy((SbObject *)f);
}

int
SbFrame_SetPrevious(SbObject *f, SbFrameObject *prev)
{
    SbFrameObject *op = (SbFrameObject *)f;

    Sb_CLEAR(op->prev);
    if (prev) {
        Sb_INCREF(prev);
        op->prev = prev;
    }
    return 0;
}

int
SbFrame_ApplyArgs(SbObject *f, SbObject *args, SbObject *kwds, SbObject *defaults)
{
    SbFrameObject *op = (SbFrameObject *)f;
    return 0;
}

/* Type initializer */

int
_SbFrame_TypeInit()
{
    SbTypeObject *tp;

    tp = SbType_New("frame", NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbFrameObject);
    tp->tp_itemsize = sizeof(SbObject *);
    tp->tp_destroy = (destructor)frame_destroy;

    SbFrame_Type = tp;
    return 0;
}
