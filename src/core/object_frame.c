#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbFrame_Type = NULL;

/*
 * C interface implementations
 */

SbFrameObject *
SbFrame_New(SbCodeObject *code, SbObject *globals, SbObject *locals)
{
    SbFrameObject *op;

    op = (SbFrameObject *)SbObject_NewVar(SbFrame_Type, code->stack_size);
    if (op) {
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
    return op;
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
SbFrame_SetPrevious(SbFrameObject *myself, SbFrameObject *prev)
{
    Sb_CLEAR(myself->prev);
    if (prev) {
        Sb_INCREF(prev);
        myself->prev = prev;
    }
    return 0;
}

int
SbFrame_ApplyArgs(SbFrameObject *myself, SbObject *args, SbObject *kwds, SbObject *defaults)
{
    return 0;
}

int
SbFrame_PushBlock(SbFrameObject *myself, const Sb_byte_t *handler, SbObject **old_sp, Sb_byte_t setup_insn)
{
    SbCodeBlock *b;

    b = (SbCodeBlock *)Sb_Malloc(sizeof(*b));
    if (!b) {
        SbErr_NoMemory();
        return -1;
    }

    b->next = myself->blocks;
    b->handler = handler;
    b->old_sp = old_sp;
    b->setup_insn = setup_insn;
    myself->blocks = b;

    return 0;
}

void
SbFrame_PopBlock(SbFrameObject *myself)
{
    SbCodeBlock *b;

    b = myself->blocks;
    /* assert(b != NULL); */
    myself->blocks = b->next;

    Sb_Free(b);
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
    tp->tp_destroy = (SbDestroyFunc)frame_destroy;

    SbFrame_Type = tp;
    return 0;
}
