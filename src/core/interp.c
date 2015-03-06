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
        SbObject *o_result;
        int i_result;
        int test_value;
        Sb_ssize_t pos;
        SbUnaryFunc ufunc;
        SbBinaryFunc bfunc;

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
            o_result = SbTuple_GetItemUnsafe(code->consts, opcode_arg);
            Sb_INCREF(o_result);
            *--sp = o_result;
            continue;

        case LoadLocals:
            o_result = frame->locals;
            Sb_INCREF(o_result);
            *--sp = o_result;
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

            name = SbTuple_GetItem(code->varnames, opcode_arg);
            o_result = SbDict_GetItemString(frame->locals, SbStr_AsStringUnsafe(name));
            goto LoadXxx_common;
        case LoadName:
            name = SbTuple_GetItem(code->names, opcode_arg);
            o_result = SbDict_GetItemString(frame->locals, SbStr_AsStringUnsafe(name));
            if (!o_result) {
                goto LoadGlobal_action;
            }
            goto LoadXxx_common;
        case LoadGlobal:
            name = SbTuple_GetItem(code->names, opcode_arg);
LoadGlobal_action:
            o_result = SbDict_GetItemString(frame->globals, SbStr_AsStringUnsafe(name));
            if (!o_result) {
                /* Try builtins */
                o_result = SbDict_GetItemString(SbModule_GetDict(Sb_ModuleBuiltin), SbStr_AsStringUnsafe(name));
            }
LoadXxx_common:
            if (o_result) {
                Sb_INCREF(o_result);
                STACK_PUSH(o_result);
                continue;
            }
            SbErr_RaiseWithObject(SbErr_NameError, name);
            reason = Reason_Error;
            break;

        case StoreFast:
            scope = frame->locals;
            tmp = code->varnames;
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
            i_result = SbDict_SetItemString(scope, SbStr_AsStringUnsafe(name), op1);
            goto XxxName_drop1_check_iresult;

        case DeleteFast:
            scope = frame->locals;
            tmp = code->varnames;
            goto DeleteXxx_common;
        case DeleteName:
            scope = frame->locals;
            tmp = code->names;
            name = SbTuple_GetItem(frame->locals, opcode_arg);
            i_result = SbDict_DelItemString(code->names, SbStr_AsStringUnsafe(name));
            if (i_result >= 0) {
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
            i_result = SbDict_DelItemString(scope, SbStr_AsStringUnsafe(name));
            goto XxxName_check_iresult;

        case LoadAttr:
            /* X -> X.attr */
            name = SbTuple_GetItem(code->names, opcode_arg);
            op1 = STACK_POP();
            o_result = SbObject_GetAttrString(op1, SbStr_AsStringUnsafe(name));
            Sb_DECREF(op1);
            if (o_result) {
                STACK_PUSH(o_result);
                continue;
            }
            /* No need to clear - SbObject_GetAttrString() doesn't raise */
            SbErr_RaiseWithObject(SbErr_AttributeError, name);
            reason = Reason_Error;
            break;
        case StoreAttr:
            /* X Y -> */
            name = SbTuple_GetItem(code->names, opcode_arg);
            op1 = STACK_POP();
            op2 = STACK_POP();
            i_result = SbObject_SetAttrString(op1, SbStr_AsStringUnsafe(name), op2);
            Sb_DECREF(op2);
            goto XxxName_drop1_check_iresult;
        case DeleteAttr:
            /* X -> */
            name = SbTuple_GetItem(code->names, opcode_arg);
            op1 = STACK_POP();
            i_result = SbObject_DelAttrString(op1, SbStr_AsStringUnsafe(name));

XxxName_drop1_check_iresult:
            Sb_DECREF(op1);
XxxName_check_iresult:
            if (i_result >= 0) {
                continue;
            }
            SbErr_Clear();
            SbErr_RaiseWithObject(SbErr_AttributeError, name);
            reason = Reason_Error;
            break;



        case BuildTuple:
            o_result = SbTuple_New(opcode_arg);
            if (!o_result) {
                goto BuildXxx_popargs;
            }
            pos = opcode_arg - 1;
            while (pos >= 0) {
                tmp = STACK_POP();
                SbTuple_SetItemUnsafe(o_result, pos, tmp);
                --pos;
            }
            STACK_PUSH(o_result);
            continue;
        case BuildList:
            o_result = SbList_New(opcode_arg);
            if (!o_result) {
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
                SbList_SetItemUnsafe(o_result, pos, tmp);
                --pos;
            }
            STACK_PUSH(o_result);
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

            o_result = SbPFunction_New((SbCodeObject *)op1, op2, frame->globals);
            goto Xxx_drop2_check_oresult;

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

                o_result = SbObject_Call(op1, op2, op3);
#if SUPPORTS_KWARGS
                Sb_DECREF(op3);
#endif
                goto Xxx_drop2_check_oresult;
            }
            break;

        case UnaryNot:
            /* X -> (not X) */
            op1 = STACK_POP();
            i_result = SbObject_Not(op1);
            Sb_DECREF(op1);
            if (i_result >= 0) {
                STACK_PUSH(SbBool_FromLong(i_result));
                continue;
            }
            reason = Reason_Error;
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
            o_result = ufunc(op1);
            goto Xxx_drop1_check_oresult;

        case CompareOp:
            /* X Y -> Y.__op__(X) */
            op1 = STACK_POP();
            op2 = STACK_POP();

            if (opcode_arg <= PyCmp_GE) {
                o_result = SbObject_Compare(op2, op1, opcode_arg);
            }
            else if (opcode_arg == PyCmp_EXC_MATCH) {
                i_result = SbErr_ExceptionMatches((SbTypeObject *)op2, op1);
                if (i_result < 0) {
                    reason = Reason_Error;
                    break;
                }
                o_result = SbBool_FromLong(i_result);
            }
            goto Xxx_drop2_check_oresult;

        case InPlaceAdd:
        case BinaryAdd:
            bfunc = &SbNumber_Add;
            goto BinaryXxx_common;
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
BinaryXxx_common:
            op1 = STACK_POP();
            op2 = STACK_POP();
            o_result = bfunc(op2, op1);
            goto Xxx_drop2_check_oresult;


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


        case RaiseVarArgs:
            /* [TraceBack] [Value] [Type] -> */
            reason = Reason_Error;

            if (opcode_arg > 0) {
                SbExceptionInfo exinfo = { NULL, NULL, NULL };

                if (opcode_arg == 3) {
                    exinfo.traceback = STACK_POP();
                }
                if (opcode_arg >= 2) {
                    exinfo.value = STACK_POP();
                }
                op1 = STACK_POP();
                if (!SbType_Check(op1)) {
                    Sb_XDECREF(exinfo.traceback);
                    Sb_XDECREF(exinfo.value);
                    Sb_XDECREF(op1);
                    SbErr_RaiseWithString(SbErr_TypeError, "only type objects can be passed at 1st parameter to raise");
                    break;
                }
                exinfo.type = (SbTypeObject *)op1;
                SbErr_Restore(&exinfo);
            }
            break;

        case SetupExcept:
        case SetupFinally:
            SbFrame_PushBlock(frame, ip + opcode_arg, sp, opcode);
            continue;

        case EndFinally:
            op1 = STACK_POP();
            /* None -> */
            if (op1 == Sb_None) {
                /* No exception occurred or it was handled */
                Sb_DECREF(op1);
                continue;
            }
            /* Reason [RetVal] -> */
            if (SbInt_CheckExact(op1)) {
                /* Restore unwind reason and go on with unwinding */
                reason = SbInt_AsLongUnsafe(op1);
                Sb_DECREF(op1);
                if (reason == Reason_Return) {
                    return_value = STACK_POP();
                }
                break;
            }
            {
                SbExceptionInfo exinfo;

                /* Type Value TraceBack -> */
                exinfo.type = (SbTypeObject *)op1;
                exinfo.value = STACK_POP();
                exinfo.traceback = STACK_POP();
                SbErr_Restore(&exinfo);
            }
            reason = Reason_Error;
            break;


        case BinarySubscript:
            /* X Y -> Y[X] */
            op1 = STACK_POP();
            op2 = STACK_POP();
            o_result = SbObject_GetItem(op2, op1);
            goto Xxx_drop2_check_oresult;

        case StoreSubscript:
            /* X Y Z -> */
            op1 = STACK_POP();
            op2 = STACK_POP();
            op3 = STACK_POP();
            i_result = SbObject_SetItem(op2, op1, op3);
            goto Xxx_drop3_check_iresult;

        case DeleteSubscript:
            /* X Y -> */
            op1 = STACK_POP();
            op2 = STACK_POP();
            i_result = SbObject_DelItem(op2, op1);
            goto Xxx_drop2_check_iresult;


        default:
            /* Not implemented. */
            SbErr_RaiseWithString(SbErr_SystemError, "opcode not implemented");
            reason = Reason_Error;
            break;

Xxx_drop2_check_oresult:
            Sb_DECREF(op2);
Xxx_drop1_check_oresult:
            Sb_DECREF(op1);
            if (o_result) {
                STACK_PUSH(o_result);
                continue;
            }
            reason = Reason_Error;
            break;

Xxx_drop3_check_iresult:
            Sb_DECREF(op3);
Xxx_drop2_check_iresult:
            Sb_DECREF(op2);
            Sb_DECREF(op1);
Xxx_check_iresult:
            if (!i_result) {
                continue;
            }
            reason = Reason_Error;
            break;
        }

        /* If we are here, something has happened (exception/return/break) */

        while (frame->blocks) {
            SbCodeBlock *b;
            Sb_byte_t insn;
            const Sb_byte_t *handler;

            b = frame->blocks;
            insn = b->setup_insn;

            /* For `continue`, there is no need to pop the block. */
            if (insn == SetupLoop && reason == Reason_Continue) {
                ip = continue_ip;
                reason = Reason_AllRightNow;
                break;
            }

            /* Drop execution stack values */
            while (sp != b->old_sp) {
                tmp = STACK_POP();
                Sb_DECREF(tmp);
            }

            /* Drop the block */
            handler = b->handler;
            SbFrame_PopBlock(frame);

            /* If it was a `break` and we hit a loop block -- drop it */
            if (insn == SetupLoop && reason == Reason_Break) {
                ip = handler;
                reason = Reason_AllRightNow;
                break;
            }

            /* The `finally` handler is always executed. */
            if (insn == SetupFinally || (insn == SetupExcept && reason == Reason_Error)) {
                tmp = Sb_None;
                if (reason == Reason_Error) {
                    SbExceptionInfo exinfo;

                    /* Both `finally` and `except`: Type Value TraceBack -> */
                    SbErr_FetchCopy(&exinfo);
                    if (!exinfo.traceback) {
                        exinfo.traceback = tmp;
                        Sb_INCREF(tmp);
                    }
                    STACK_PUSH(exinfo.traceback);
                    if (!exinfo.value) {
                        exinfo.value = tmp;
                        Sb_INCREF(tmp);
                    }
                    STACK_PUSH(exinfo.value);
                    STACK_PUSH((SbObject *)exinfo.type);
                }
                else {
                    if (reason == Reason_Return) {
                        STACK_PUSH(return_value);
                        return_value = NULL;
                    }

                    tmp = SbInt_FromLong(reason);
                    STACK_PUSH(tmp);
                }
                ip = handler;
                reason = Reason_AllRightNow;
                break;
            }

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

    SbInterp_TopFrame = frame->prev;
    return return_value;
}

