#include "snakebed.h"
#include "opcode.h"

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
    SbObject_Destroy((SbObject *)f);
}

static int
frame_populate_args(SbFrameObject *f, SbObject *args)
{
    Sb_ssize_t pos, count;

    count = SbTuple_GetSize(args);
    if (count != f->code->arg_count) {
        /* raise ArgumentError */
        return -1;
    }

    for (pos = 0; pos < count; ++pos) {
        SbObject *arg;
        
        arg = SbTuple_GetItemUnsafe(args, pos);
        /* Not sure -- this may or may not have been checked already */
        if (!arg) {
            /* raise ArgumentError? */
            return -1;
        }
        f->vars[pos] = arg;
    }

    return 0;
}

SbObject *
SbFrame_Execute(SbFrameObject *f, SbObject *args, SbObject *kwargs)
{
    SbObject *rv;
    const Sb_byte_t *bytecode;
    Sb_ssize_t bytecode_len;
    int run = 1;

    if (frame_populate_args(f, args) < 0) {
        return NULL;
    }

    bytecode = SbStr_AsStringUnsafe(f->code->code);
    bytecode_len = SbStr_GetSizeUnsafe(f->code->code);
    rv = NULL;

    while (run) {
        Sb_byte_t opcode;
        unsigned opcode_arg;
        SbObject **sp = f->sp;
        SbObject *tmp;
        SbObject *tmp2;

        opcode = *bytecode++;
        if (opcode >= HaveArgument) {
            opcode_arg = bytecode[0];
            opcode_arg = (opcode_arg << 8) | bytecode[1];
            bytecode += 2;
        }
        switch (opcode) {
        case Nop:
            break;
        case PopTop:
            tmp = *sp++;
            Sb_DECREF(tmp);
            break;
        case DupTop:
            tmp = *sp;
            Sb_INCREF(tmp);
            *--sp = tmp;
            break;
        case RotTwo:
            tmp = sp[0];
            sp[0] = sp[1];
            sp[1] = tmp;
            break;
        case RotThree:
            tmp = sp[0];
            sp[0] = sp[1];
            sp[1] = sp[2];
            sp[2] = tmp;
            break;
        case RotFour:
            tmp = sp[0];
            sp[0] = sp[1];
            sp[1] = sp[2];
            sp[2] = sp[3];
            sp[3] = tmp;
            break;

            /* Unary operations */
        case UnaryPositive:
        case UnaryNegative:
        case UnaryNot:
        case UnaryConvert:
        case UnaryInvert:
            run = 0;
            break;

            /* Binary operations */
        case BinaryPower:
        case InPlacePower:
        case BinaryMultiply:
        case InPlaceMultiply:
        case BinaryDivide:
        case InPlaceDivide:
        case BinaryModulo:
        case InPlaceModulo:
        case BinaryAdd:
        case InPlaceAdd:
        case BinarySubtract:
        case InPlaceSubtract:
        case BinarySubscript:
        case BinaryFloorDivide:
        case InPlaceFloorDivide:
        case BinaryTrueDivide:
        case InPlaceTrueDivide:
            run = 0;
            break;

        case ReturnValue:
            rv = *sp++;
            run = 0;
            break;

        case LoadConst:
            tmp = SbTuple_GetItemUnsafe(f->code->consts, opcode_arg);
            Sb_INCREF(tmp);
            *--sp = tmp;
            break;

        case LoadFast:
            tmp = f->vars[opcode_arg];
            Sb_INCREF(tmp);
            *--sp = tmp;
            break;
        case StoreFast:
            tmp = f->vars[opcode_arg];
            f->vars[opcode_arg] = *sp++;
            Sb_XDECREF(tmp);
            break;
        case DeleteFast:
            tmp = f->vars[opcode_arg];
            f->vars[opcode_arg] = NULL;
            Sb_XDECREF(tmp);
            break;

        case LoadGlobal:
        case StoreGlobal:
        case DeleteGlobal:
            run = 0;
            break;

        case LoadName:
        case StoreName:
        case DeleteName:
            run = 0;
            break;

        case LoadAttr:
        case StoreAttr:
        case DeleteAttr:
            run = 0;
            break;

        default:
            run = 0;
            break;
        }
        f->sp = sp;
    }

    return rv;
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
