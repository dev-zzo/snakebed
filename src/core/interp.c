#include "snakebed.h"
#include "interp.h"
#include "opcode.h"

SbFrameObject *SbInterp_TopFrame = NULL;

#define STACK_PUSH(x) *--sp = (x)
#define STACK_POP() *sp++
#define STACK_TOP() *sp

enum SbUnwindReason {
    Reason_Unknown,

    Reason_AllRightNow, /* We have sucessfully handled whatever was there */

    Reason_Break,       /* A break `statement` has been executed */
    Reason_Continue,    /* A `continue` statement has been executed */
    Reason_Return,      /* A `return` statement has been executed */

    Reason_Error, /* An exception has been raised (probably) */
};

SbObject *
SbInterp_Execute(SbFrameObject *frame)
{
    SbObject *return_value = NULL;
    SbCodeObject *code;
    const Sb_byte_t *ip;
    SbObject **sp;
    SbObject **sp_base;
    enum SbUnwindReason reason;
    const Sb_byte_t *continue_ip;

    /* Link the new frame into frame chain. */
    SbFrame_SetPrevious(frame, SbInterp_TopFrame);
    SbInterp_TopFrame = frame;
    code = frame->code;

    /* Setup initial values for sp and ip. */
    sp_base = sp = frame->sp;
    ip = frame->ip;

    /* Loop until a return is executed or an exception is raised. */
    for (;;) {
        SbOpcode opcode;
        unsigned opcode_arg;
        SbObject *tmp;
        SbObject *op1, *op2, *op3;
        SbObject *scope;
        SbObject *name;
        SbObject *result;
        int i_result;
        int test_value;
        Sb_ssize_t pos;
        SbUnaryFunc ufunc;
        SbBinaryFunc bfunc;
        int failure;

        reason = Reason_Unknown;

        opcode = (SbOpcode)(*ip++);
        if (opcode >= HaveArgument) {
            opcode_arg = ip[0];
            opcode_arg |= ip[1] << 8;
            ip += 2;
        }

        switch (opcode) {

        case Nop:
            /* No operation. */
            continue;

        case PopTop:
            /* X -> */
            op1 = STACK_POP();
            Sb_DECREF(op1);
            continue;

        case DupTop:
            /* X -> X X */
            op1 = STACK_TOP();
            Sb_INCREF(op1);
            *--sp = op1;
            continue;

        case RotTwo:
            /* X Y -> Y X */
            tmp = sp[0];
            sp[0] = sp[1];
            sp[1] = tmp;
            continue;
        case RotThree:
            /* X Y Z -> Y Z X */
            tmp = sp[0];
            sp[0] = sp[1];
            sp[1] = sp[2];
            sp[2] = tmp;
            continue;
        case RotFour:
            /* X Y Z W -> Y Z W X */
            tmp = sp[0];
            sp[0] = sp[1];
            sp[1] = sp[2];
            sp[2] = sp[3];
            sp[3] = tmp;
            continue;

        case LoadConst:
            result = SbTuple_GetItemUnsafe(code->consts, opcode_arg);
            Sb_INCREF(result);
            *--sp = result;
            continue;

        case LoadLocals:
            result = frame->locals;
            Sb_INCREF(result);
            *--sp = result;
            continue;

        case JumpForward:
            ip += opcode_arg;
            continue;

        case JumpAbsolute:
            ip = SbStr_AsStringUnsafe(code->code) + opcode_arg;
            continue;

        case JumpIfFalseOrPop:
            test_value = 0;
            goto JumpIfXxxOrPop;
        case JumpIfTrueOrPop:
            test_value = 1;
JumpIfXxxOrPop:
            op1 = STACK_TOP();
            i_result = SbObject_IsTrue(op1);
            if (i_result < 0) {
                reason = Reason_Error;
                break;
            }
            if (i_result == test_value) {
                ip = SbStr_AsStringUnsafe(code->code) + opcode_arg;
            }
            else {
                --sp;
                Sb_DECREF(op1);
            }
            continue;

        case PopJumpIfFalse:
            test_value = 0;
            goto PopJumpIfXxx;
        case PopJumpIfTrue:
            test_value = 1;
PopJumpIfXxx:
            op1 = STACK_POP();
            i_result = SbObject_IsTrue(op1);
            Sb_DECREF(op1);
            if (i_result < 0) {
                break;
            }
            if (i_result == test_value) {
                ip = SbStr_AsStringUnsafe(code->code) + opcode_arg;
            }
            continue;

        case LoadFast:
            if (!frame->locals) {
                reason = Reason_Error;
                break;
            }

            name = SbTuple_GetItem(code->fastnames, opcode_arg);
            result = SbDict_GetItemString(frame->locals, SbStr_AsStringUnsafe(name));
            goto LoadXxx_common;
        case LoadName:
            name = SbTuple_GetItem(code->names, opcode_arg);
            result = SbDict_GetItemString(frame->locals, SbStr_AsStringUnsafe(name));
            if (!result) {
                goto LoadGlobal_action;
            }
            goto LoadXxx_common;
        case LoadGlobal:
            name = SbTuple_GetItem(code->names, opcode_arg);
LoadGlobal_action:
            result = SbDict_GetItemString(frame->globals, SbStr_AsStringUnsafe(name));
            if (!result) {
                /* Try builtins */
                result = SbDict_GetItemString(SbModule_GetDict(Sb_ModuleBuiltin), SbStr_AsStringUnsafe(name));
            }
LoadXxx_common:
            if (result) {
                Sb_INCREF(result);
                STACK_PUSH(result);
                continue;
            }
            SbErr_RaiseWithObject(SbErr_NameError, name);
            reason = Reason_Error;
            break;

        case StoreFast:
            scope = frame->locals;
            tmp = code->fastnames;
            goto StoreXxx_common;
        case StoreName:
            scope = frame->locals;
            tmp = code->names;
            goto StoreXxx_common;
        case StoreGlobal:
            scope = frame->globals;
            tmp = code->names;
StoreXxx_common:
            name = SbTuple_GetItem(tmp, opcode_arg);
            op1 = STACK_POP();
            failure = SbDict_SetItemString(scope, SbStr_AsStringUnsafe(name), op1) < 0;
            Sb_DECREF(op1);
            if (!failure) {
                continue;
            }
            SbErr_Clear();
            SbErr_RaiseWithObject(SbErr_NameError, name);
            reason = Reason_Error;
            break;

        case DeleteFast:
            scope = frame->locals;
            tmp = code->fastnames;
            goto DeleteXxx_common;
        case DeleteName:
            scope = frame->locals;
            tmp = code->names;
            name = SbTuple_GetItem(frame->locals, opcode_arg);
            failure = SbDict_DelItemString(code->names, SbStr_AsStringUnsafe(name)) < 0;
            if (!failure) {
                continue;
            }
            if (!SbErr_Occurred() || !SbErr_ExceptionMatches(SbErr_Occurred(), (SbObject *)SbErr_KeyError)) {
                break;
            }
            SbErr_Clear();
            /* Fall through */
        case DeleteGlobal:
            scope = frame->globals;
            tmp = code->names;
DeleteXxx_common:
            name = SbTuple_GetItem(tmp, opcode_arg);
            failure = SbDict_DelItemString(scope, SbStr_AsStringUnsafe(name)) < 0;
            if (!failure) {
                continue;
            }
            SbErr_Clear();
            SbErr_RaiseWithObject(SbErr_NameError, name);
            reason = Reason_Error;
            break;

        case LoadAttr:
            /* X -> X.attr */
            name = SbTuple_GetItem(code->names, opcode_arg);
            op1 = STACK_POP();
            result = SbObject_GetAttrString(op1, SbStr_AsStringUnsafe(name));
            Sb_DECREF(op1);
            if (result) {
                STACK_PUSH(result);
                continue;
            }
            /* No need to clear */
            SbErr_RaiseWithObject(SbErr_AttributeError, name);
            reason = Reason_Error;
            break;
        case StoreAttr:
            /* X Y -> */
            name = SbTuple_GetItem(code->names, opcode_arg);
            op1 = STACK_POP();
            op2 = STACK_POP();
            failure = SbObject_SetAttrString(op1, SbStr_AsStringUnsafe(name), op2) < 0;
            Sb_DECREF(op2);
            Sb_DECREF(op1);
            if (!failure) {
                continue;
            }
            SbErr_Clear();
            SbErr_RaiseWithObject(SbErr_AttributeError, name);
            reason = Reason_Error;
            break;
        case DeleteAttr:
            /* X -> */
            name = SbTuple_GetItem(code->names, opcode_arg);
            op1 = STACK_POP();
            failure = SbObject_DelAttrString(op1, SbStr_AsStringUnsafe(name)) < 0;
            Sb_DECREF(op1);
            if (!failure) {
                continue;
            }
            SbErr_Clear();
            SbErr_RaiseWithObject(SbErr_AttributeError, name);
            reason = Reason_Error;
            break;

        case BuildTuple:
            result = SbTuple_New(opcode_arg);
            if (!result) {
                goto BuildXxx_popargs;
            }
            pos = opcode_arg - 1;
            while (pos >= 0) {
                tmp = STACK_POP();
                SbTuple_SetItemUnsafe(result, pos, tmp);
                --pos;
            }
            STACK_PUSH(result);
            continue;
        case BuildList:
            result = SbList_New(opcode_arg);
            if (!result) {
BuildXxx_popargs:
                while (opcode_arg--) {
                    tmp = STACK_POP();
                    Sb_DECREF(tmp);
                }
                reason = Reason_Error;
                break;
            }
            pos = opcode_arg - 1;
            while (pos >= 0) {
                tmp = STACK_POP();
                SbList_SetItemUnsafe(result, pos, tmp);
                --pos;
            }
            STACK_PUSH(result);
            continue;

        case MakeFunction:
            /* C DN DN-1 ... -> F */
            op1 = STACK_POP();
            op2 = SbTuple_New(opcode_arg);
            if (!op2) {
                goto BuildXxx_popargs;
            }

            pos = opcode_arg - 1;
            while (pos >= 0) {
                tmp = STACK_POP();
                SbTuple_SetItemUnsafe(op2, pos, tmp);
                --pos;
            }

            result = SbPFunction_New((SbCodeObject *)op1, op2, frame->globals);
            Sb_DECREF(op2);
            Sb_DECREF(op1);
            if (result) {
                STACK_PUSH(result);
                continue;
            }
            reason = Reason_Error;
            break;

        case CallFunction:
            {
                Sb_ssize_t posargs_passed, kwargs_passed;

                kwargs_passed = (opcode_arg >> 8) & 0xFF;
#if SUPPORTS_KWARGS
                /* On the stack, the opcode finds the keyword parameters first.
                For each keyword argument, the value is on top of the key.
                */
                op3 = SbDict_New();
                if (!op3) {
                    reason = Reason_Error;
                    break;
                }
                pos = 0;
                while (pos < kwargs_passed) {
                    SbObject *key;
                    SbObject *value;

                    value = STACK_POP();
                    key = STACK_POP();
                    SbDict_SetItemString(op3, SbStr_AsString(key), value);
                    Sb_DECREF(key);
                    Sb_DECREF(value);
                    ++pos;
                }
#else
                if (kwargs_passed) {
                    /* Whine terribly */
                    reason = Reason_Error;
                    break;
                }
#endif
                /* Below the keyword parameters, the positional parameters 
                are on the stack, with the right-most parameter on top.
                */
                posargs_passed = opcode_arg & 0xFF;
                op2 = SbTuple_New(posargs_passed);
                if (!op2) {
#if SUPPORTS_KWARGS
                    Sb_DECREF(op3);
#endif
                    reason = Reason_Error;
                    break;
                }
                pos = posargs_passed - 1;
                while (pos >= 0) {
                    SbTuple_SetItemUnsafe(op2, pos, STACK_POP());
                    --pos;
                }

                op1 = STACK_POP();

                result = SbObject_Call(op1, op2, op3);
#if SUPPORTS_KWARGS
                Sb_DECREF(op3);
#endif
                Sb_DECREF(op2);
                Sb_DECREF(op1);
                if (result) {
                    STACK_PUSH(result);
                    continue;
                }
                reason = Reason_Error;
            }
            break;

        case UnaryPositive:
            /* X -> type(X).__pos__(X) */
            ufunc = SbNumber_Positive;
            goto UnaryXxx_common;
        case UnaryNegative:
            /* X -> type(X).__neg__(X) */
            ufunc = SbNumber_Negative;
            goto UnaryXxx_common;
        case UnaryInvert:
            /* X -> type(X).__invert__(X) */
            ufunc = SbNumber_Invert;
UnaryXxx_common:
            op1 = STACK_POP();
            result = ufunc(op1);
            Sb_DECREF(op1);
            if (result) {
                STACK_PUSH(result);
                continue;
            }
            reason = Reason_Error;
            break;

        case CompareOp:
            /* X Y -> Y.__op__(X) */
            op1 = STACK_POP();
            op2 = STACK_POP();
            result = SbObject_Compare(op2, op1, opcode_arg);
            Sb_DECREF(op2);
            Sb_DECREF(op1);
            if (result) {
                STACK_PUSH(result);
                continue;
            }
            reason = Reason_Error;
            break;

        case InPlaceAdd:
        case BinaryAdd:
            bfunc = &SbNumber_Add;
BinaryXxx_common:
            op1 = STACK_POP();
            op2 = STACK_POP();
            result = bfunc(op2, op1);
            Sb_DECREF(op2);
            Sb_DECREF(op1);
            failure = result == NULL;
            if (!failure) {
                STACK_PUSH(result);
                continue;
            }
            reason = Reason_Error;
            break;
        case InPlaceSubtract:
        case BinarySubtract:
            bfunc = &SbNumber_Subtract;
            goto BinaryXxx_common;
        case InPlaceMultiply:
        case BinaryMultiply:
            bfunc = &SbNumber_Multiply;
            goto BinaryXxx_common;
        case InPlaceDivide:
        case BinaryDivide:
            bfunc = &SbNumber_Divide;
            goto BinaryXxx_common;
        case InPlaceFloorDivide:
        case BinaryFloorDivide:
            bfunc = &SbNumber_FloorDivide;
            goto BinaryXxx_common;
        case InPlaceTrueDivide:
        case BinaryTrueDivide:
            bfunc = &SbNumber_TrueDivide;
            goto BinaryXxx_common;
        case InPlaceModulo:
        case BinaryModulo:
            bfunc = &SbNumber_Remainder;
            goto BinaryXxx_common;
        case InPlaceAnd:
        case BinaryAnd:
            bfunc = &SbNumber_And;
            goto BinaryXxx_common;
        case InPlaceXor:
        case BinaryXor:
            bfunc = &SbNumber_Xor;
            goto BinaryXxx_common;
        case InPlaceOr:
        case BinaryOr:
            bfunc = &SbNumber_Or;
            goto BinaryXxx_common;
        case InPlaceLeftShift:
        case BinaryLeftShift:
            bfunc = &SbNumber_Or;
            goto BinaryXxx_common;
        case InPlaceRightShift:
        case BinaryRightShift:
            bfunc = &SbNumber_Or;
            goto BinaryXxx_common;


        case ReturnValue:
            /* NOTE: Executing this instruction may traverse block boundaries */
            return_value = STACK_POP();
            reason = Reason_Return;
            break;


        case SetupLoop:
            SbFrame_PushBlock(frame, ip + opcode_arg, sp, opcode);
            continue;

        case PopBlock:
            while (sp != frame->blocks->old_sp) {
                tmp = STACK_POP();
                Sb_DECREF(tmp);
            }
            SbFrame_PopBlock(frame);
            continue;

        case ContinueLoop:
            /* NOTE: Executing this instruction may traverse block boundaries */
            /* NOTE: `continue` is forbidden in `finally` clause */
            continue_ip = SbStr_AsStringUnsafe(code->code) + opcode_arg;
            reason = Reason_Continue;
            break;

        case BreakLoop:
            /* NOTE: Executing this instruction may traverse block boundaries */
            reason = Reason_Break;
            break;

        default:
            /* Not implemented. */
            break;
        }

        /* If we are here, something has happened (exception/return/break) */
        while (frame->blocks) {
            SbCodeBlock *b;
            Sb_byte_t insn;

            b = frame->blocks;
            insn = b->setup_insn;

            /* For `continue`, there is no need to pop the block. */
            if (insn == SetupLoop && reason == Reason_Continue) {
                ip = continue_ip;
                reason = Reason_AllRightNow;
                break;
            }

            while (sp != b->old_sp) {
                tmp = STACK_POP();
                Sb_DECREF(tmp);
            }

            if (insn == SetupLoop && reason == Reason_Break) {
                ip = b->handler;
                reason = Reason_AllRightNow;
                SbFrame_PopBlock(frame);
                break;
            }

            SbFrame_PopBlock(frame);
        }

        if (reason == Reason_AllRightNow) {
            continue;
        }

        /* It's either return or exception propagation. */
        break;
    }

    while (sp != sp_base) {
        SbObject *tmp;

        tmp = STACK_POP();
        Sb_DECREF(tmp);
    }

    return return_value;
}

