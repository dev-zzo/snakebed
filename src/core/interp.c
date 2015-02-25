#include "snakebed.h"
#include "interp.h"
#include "opcode.h"

SbFrameObject *SbInterp_TopFrame;

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

int 
SbInterp_PushFrame(SbFrameObject *f, SbObject *args, SbObject *kwargs)
{
    if (frame_populate_args(f, args) < 0) {
        return -1;
    }

    SbInterp_TopFrame = f;
    return 0;
}


int
SbInterp_ExecuteNext(void)
{
    SbObject **sp;
    SbObject *tmp;
    SbFrameObject *frame;
    const Sb_byte_t *bytecode;
    Sb_byte_t opcode;
    unsigned opcode_arg;
    int failed = 0;

    frame = SbInterp_TopFrame;
    sp = frame->sp;
    bytecode = frame->ip;
    opcode = *bytecode++;
    if (opcode >= HaveArgument) {
        opcode_arg = bytecode[0];
        opcode_arg = (opcode_arg << 8) | bytecode[1];
        bytecode += 2;
    }

    switch (opcode) {
    case Nop:
        /* No operation. */
        break;

    case PopTop:
        /* X -> */
        tmp = *sp++;
        Sb_DECREF(tmp);
        break;

    case DupTop:
        /* X -> X X */
        tmp = *sp;
        Sb_INCREF(tmp);
        *--sp = tmp;
        break;

    case RotTwo:
        /* X Y -> Y X */
        tmp = sp[0];
        sp[0] = sp[1];
        sp[1] = tmp;
        break;
    case RotThree:
        /* X Y Z -> Y Z X */
        tmp = sp[0];
        sp[0] = sp[1];
        sp[1] = sp[2];
        sp[2] = tmp;
        break;
    case RotFour:
        /* X Y Z W -> Y Z W X */
        tmp = sp[0];
        sp[0] = sp[1];
        sp[1] = sp[2];
        sp[2] = sp[3];
        sp[3] = tmp;
        break;

        /* Unary operations */

    case UnaryPositive:
        /* X -> type(X).__pos__(X) */
        {
            SbObject *result;

            result = SbObject_CallMethod(sp[0], "__pos__", NULL, NULL);
            tmp = sp[0];
            sp[0] = result;
            Sb_DECREF(tmp);
        }
        break;
    case UnaryNegative:
        /* X -> type(X).__neg__(X) */
        {
            SbObject *result;

            result = SbObject_CallMethod(sp[0], "__neg__", NULL, NULL);
            tmp = sp[0];
            sp[0] = result;
            Sb_DECREF(tmp);
        }
        break;
    case UnaryNot:
    case UnaryConvert:
        break;
    case UnaryInvert:
        /* X -> type(X).__invert__(X) */
        {
            SbObject *result;

            result = SbObject_CallMethod(sp[0], "__invert__", NULL, NULL);
            tmp = sp[0];
            sp[0] = result;
            Sb_DECREF(tmp);
        }
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
        break;

    case ReturnValue:
        tmp = *sp++;
        /* TODO: handle. */
        break;

    case LoadConst:
        tmp = SbTuple_GetItemUnsafe(frame->code->consts, opcode_arg);
        Sb_INCREF(tmp);
        *--sp = tmp;
        break;

        /* Operations on local variables */

    case LoadFast:
        tmp = frame->vars[opcode_arg];
        if (tmp) {
            Sb_INCREF(tmp);
        }
        else {
            /* Unbounded local */
        }
        *--sp = tmp;
        break;
    case StoreFast:
        tmp = frame->vars[opcode_arg];
        frame->vars[opcode_arg] = *sp++;
        Sb_XDECREF(tmp);
        break;
    case DeleteFast:
        tmp = frame->vars[opcode_arg];
        frame->vars[opcode_arg] = NULL;
        if (tmp) {
            Sb_DECREF(tmp);
        }
        else {
            /* Unbounded local */
        }
        break;

    case LoadGlobal:
    case StoreGlobal:
    case DeleteGlobal:
        break;

    case LoadName:
    case StoreName:
    case DeleteName:
        break;

    case LoadAttr:
        /* X -> X.attr */
        {
            SbObject *attr_name;
            SbObject *attr;

            attr_name = SbTuple_GetItem(frame->code->names, opcode_arg);
            attr = SbObject_GetAttrString(sp[0], SbStr_AsStringUnsafe(attr_name));
            if (!attr) {
                SbErr_RaiseWithObject(SbErr_AttributeError, attr_name);
            }
            tmp = sp[0];
            sp[0] = attr;
            Sb_XDECREF(tmp);
        }
        break;

    case StoreAttr:
        /* X Y -> */
        {
            SbObject *attr_name;
            int result;

            attr_name = SbTuple_GetItem(frame->code->names, opcode_arg);
            result = SbObject_SetAttrString(sp[0], SbStr_AsStringUnsafe(attr_name), sp[1]);
            /* The exception should be set already */
            Sb_DECREF(sp[0]);
            Sb_DECREF(sp[1]);
            sp += 2;
        }
        break;

    case DeleteAttr:
        /* X -> */
        {
            SbObject *attr_name;
            int result;

            attr_name = SbTuple_GetItem(frame->code->names, opcode_arg);
            result = SbObject_DelAttrString(sp[0], SbStr_AsStringUnsafe(attr_name));
            /* The exception should be set already */
            Sb_DECREF(sp[0]);
            sp += 1;
        }
        break;

    case BuildTuple:
    case BuildList:
    case BuildMap:
    case BuildSet:
        break;

    case JumpForward:
        bytecode += opcode_arg;
        break;

    case JumpIfFalseOrPop:
    case JumpIfTrueOrPop:
    case PopJumpIfFalse:
    case PopJumpIfTrue:
        break;

    case JumpAbsolute:
        bytecode = SbStr_AsStringUnsafe(frame->code->code) + opcode_arg;
        break;

    default:
        /* Not implemented. */
        break;
    }

    /* Update frame */
    frame->sp = sp;
    frame->ip = bytecode;

    /* Check for errors/exceptions set */
    if (SbErr_Occurred()) {
    }

    return 0;
}
