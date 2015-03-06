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

/* https://docs.python.org/2/reference/compound_stmts.html#function-definitions
 */
int
SbFrame_ApplyArgs(SbFrameObject *myself, SbObject *args, SbObject *kwds, SbObject *defaults)
{
    Sb_ssize_t arg_pos;
    Sb_ssize_t expected_arg_count;
    Sb_ssize_t passed_arg_count = 0;
    Sb_ssize_t defaults_count = 0;
    Sb_ssize_t va_name_pos;
    SbCodeObject *code;
    SbObject *locals;
    SbObject *o;
    SbObject *varargs;

    code = myself->code;
    locals = myself->locals;
    expected_arg_count = code->arg_count;
    if (args) {
        passed_arg_count = SbTuple_GetSizeUnsafe(args);
    }

    /* Apply default args, if any */
    if (defaults) {
        Sb_ssize_t offset;

        defaults_count = SbTuple_GetSizeUnsafe(defaults);
        offset = expected_arg_count - defaults_count;

        for (arg_pos = 0; arg_pos < defaults_count; ++arg_pos) {
            o = SbTuple_GetItemUnsafe(defaults, arg_pos);
            SbDict_SetItemString(locals, 
                SbStr_AsStringUnsafe(SbTuple_GetItemUnsafe(code->varnames, offset + arg_pos)),
                o);
        }
    }

    /* Verify passed positional arg count is enough */
    if (passed_arg_count + defaults_count < expected_arg_count) {
        SbErr_RaiseWithFormat(SbErr_TypeError,
            "function expects minimum %d args (%d passed)",
            expected_arg_count - defaults_count,
            passed_arg_count);
        return -1;
    }

    /* It seems this object needs to be created anyway. */
    va_name_pos = expected_arg_count;
    if (myself->code->flags & SbCode_VARARGS) {
        Sb_ssize_t varargs_count = expected_arg_count - passed_arg_count;

        if (varargs_count < 0) {
            varargs_count = 0;
        }
        varargs = SbTuple_New(varargs_count);
        if (!varargs) {
            return -1;
        }
        SbDict_SetItemString(locals,
            SbStr_AsStringUnsafe(SbTuple_GetItemUnsafe(code->varnames, va_name_pos)),
            varargs);
        Sb_DECREF(varargs);
        /* `varargs` is a borrowed refernce now. */
        va_name_pos += 1;
    }

    /* Apply positional args, if any */
    if (args) {
        /* Fill in all expected positional args */
        for (arg_pos = 0; arg_pos < expected_arg_count; ++arg_pos) {
            o = SbTuple_GetItemUnsafe(args, arg_pos);
            SbDict_SetItemString(locals, 
                SbStr_AsStringUnsafe(SbTuple_GetItemUnsafe(code->varnames, arg_pos)),
                o);
        }

        /* If the form `*identifier` is present, it is initialized to a tuple
         * receiving any excess positional parameters, defaulting to the empty tuple. */
        while (arg_pos < passed_arg_count) {
            o = SbTuple_GetItemUnsafe(args, arg_pos);
            SbTuple_SetItemUnsafe(varargs, arg_pos - expected_arg_count, o);
            ++arg_pos;
        }
    }

#if SUPPORTS_KWARGS
    /* If the form `**identifier` is present, it is initialized to a new dictionary 
     * receiving any excess keyword arguments, defaulting to a new empty dictionary. */
    if (myself->code->flags & SbCode_VARKWDS) {
        if (kwds) {
            o = kwds;
            Sb_INCREF(o);
        }
        else {
            o = SbDict_New();
            if (!o) {
                return -1;
            }
        }
        SbDict_SetItemString(locals,
            SbStr_AsStringUnsafe(SbTuple_GetItemUnsafe(code->varnames, va_name_pos)),
            o);
        Sb_DECREF(o);
    }
#endif

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
