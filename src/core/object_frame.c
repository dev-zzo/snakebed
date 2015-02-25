#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbFrame_Type = NULL;

/*
 * C interface implementations
 */

SbObject *
SbFrame_New(SbCodeObject *code, SbFrameObject *prev)
{
    SbObject *p;

    p = (SbObject *)SbObject_NewVar(SbFrame_Type, code->localvars_count + code->stack_size);
    if (p) {
        SbFrameObject *op = (SbFrameObject *)p;
        Sb_INCREF(code);
        op->code = code;
        if (prev) {
            Sb_INCREF(prev);
            op->prev = prev;
        }
        /* stack pointer points just outside the stack */
        op->sp = &op->vars[code->localvars_count + code->stack_size];
    }
    return p;
}

static void
frame_destroy(SbFrameObject *f)
{
    Sb_XDECREF(f->code);
    Sb_XDECREF(f->vars);
    Sb_XDECREF(f->prev);
    SbObject_DefaultDestroy((SbObject *)f);
}

int
_SbFrame_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("frame", NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbFrameObject);
    tp->tp_itemsize = sizeof(SbObject *);
    tp->tp_destroy = (destructor)frame_destroy;

    SbCFunction_Type = tp;
    return 0;
}
