#include "snakebed.h"
#include "interp.h"
#include "opcode.h"

/* Ref: https://docs.python.org/2/library/dis.html */

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
    SbObject *locals, *globals;
    SbObject *names;

    /* Link the new frame into frame chain. */
    SbFrame_SetPrevious(frame, SbInterp_TopFrame);
    SbInterp_TopFrame = frame;
    code = frame->code;

    /* Spill frame/code internals onto stack */
    locals = frame->locals;
    globals = frame->globals;
    names = code->names;

    /* Setup initial values for sp and ip. */
    sp_base = sp = frame->sp;
    ip = frame->ip;

    /* Loop until a return is executed or an exception is raised. */
    for (;;) {
        SbOpcode opcode;
        unsigned opcode_arg;
        SbObject *scope;
        const char *name;
        int test_value;
        Sb_ssize_t pos;
        SbUnaryFunc ufunc;
        SbBinaryFunc bfunc;
        const Sb_byte_t *continue_ip;

#ifdef _DEBUG
        /* Wipe previous values */
        scope = NULL;
        name = NULL;
        ufunc = NULL;
        bfunc = NULL;
        continue_ip = NULL;
#endif

        reason = Reason_Error;

        opcode = (SbOpcode)(*ip++);
        if (opcode >= HaveArgument) {
            opcode_arg = ip[0];
            opcode_arg |= ip[1] << 8;
            ip += 2;
        }

        {
            SbObject *tmp;
            SbObject *op1, *op2, *op3, *op4;
            SbObject *o_result;
            int i_result;

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
                STACK_PUSH(o_result);
                continue;

            case LoadLocals:
                o_result = locals;
                Sb_INCREF(o_result);
                STACK_PUSH(o_result);
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
                    goto Xxx_check_error;
                }
                if (i_result == test_value) {
                    ip = SbStr_AsStringUnsafe(code->code) + opcode_arg;
                }
                else {
                    ++sp;
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
                    goto Xxx_check_error;
                }
                if (i_result == test_value) {
                    ip = SbStr_AsStringUnsafe(code->code) + opcode_arg;
                }
                continue;

            case LoadFast:
                /* Tries: locals */
                if (locals) {
                    name = SbStr_AsString(SbTuple_GetItem(code->varnames, opcode_arg));
                    o_result = SbDict_GetItemString(locals, name);
                    if (o_result) {
                        goto Xxx_incref_push_continue;
                    }
                }
                SbErr_RaiseWithFormat(SbErr_UnboundLocalError, "name '%s' used before being bound", SbStr_AsStringUnsafe(name));
                break;
            case LoadName:
                /* Tries: locals, globals, builtins */
                name = SbStr_AsString(SbTuple_GetItem(names, opcode_arg));
                if (locals) {
                    o_result = SbDict_GetItemString(locals, name);
                    if (o_result) {
                        goto Xxx_incref_push_continue;
                    }
                }
                o_result = SbDict_GetItemString(globals, name);
                if (o_result) {
                    goto Xxx_incref_push_continue;
                }
                o_result = SbDict_GetItemString(SbModule_GetDict(Sb_ModuleBuiltin), name);
                if (o_result) {
                    goto Xxx_incref_push_continue;
                }
                SbErr_RaiseWithFormat(SbErr_NameError, "name '%s' not found", SbStr_AsStringUnsafe(name));
                break;
            case LoadGlobal:
                /* Tries: globals, builtins */
                name = SbStr_AsString(SbTuple_GetItem(names, opcode_arg));
                o_result = SbDict_GetItemString(globals, name);
                if (o_result) {
                    goto Xxx_incref_push_continue;
                }
                o_result = SbDict_GetItemString(SbModule_GetDict(Sb_ModuleBuiltin), name);
                if (o_result) {
                    goto Xxx_incref_push_continue;
                }
                SbErr_RaiseWithFormat(SbErr_NameError, "global name '%s' not found", name);
                break;

            case StoreFast:
                scope = locals;
                tmp = code->varnames;
                goto StoreXxx_common;
            case StoreName:
                scope = locals;
                tmp = names;
                goto StoreXxx_common;
            case StoreGlobal:
                scope = globals;
                tmp = names;
StoreXxx_common:
                name = SbStr_AsString(SbTuple_GetItem(tmp, opcode_arg));
                op1 = STACK_POP();
                i_result = SbDict_SetItemString(scope, name, op1);
                goto XxxName_drop1_check_iresult;

            case DeleteFast:
                scope = locals;
                tmp = code->varnames;
                goto DeleteXxx_common;
            case DeleteName:
                name = SbStr_AsString(SbTuple_GetItem(names, opcode_arg));
                i_result = SbDict_DelItemString(locals, name);
                if (i_result >= 0) {
                    continue;
                }
                if (!SbErr_Occurred() || !SbErr_ExceptionMatches(SbErr_Occurred(), (SbObject *)SbErr_KeyError)) {
                    break;
                }
                SbErr_Clear();
                /* Fall through */
            case DeleteGlobal:
                scope = globals;
                tmp = names;
DeleteXxx_common:
                name = SbStr_AsString(SbTuple_GetItem(tmp, opcode_arg));
                i_result = SbDict_DelItemString(scope, name);
                goto XxxName_check_iresult;

            case LoadAttr:
                /* X -> X.attr */
                name = SbStr_AsString(SbTuple_GetItem(names, opcode_arg));
                op1 = STACK_POP();
                o_result = SbObject_GetAttrString(op1, name);
                Sb_DECREF(op1);
                if (o_result) {
                    STACK_PUSH(o_result);
                    continue;
                }
                /* No need to clear - SbObject_GetAttrString() doesn't raise */
                SbErr_RaiseWithString(SbErr_AttributeError, name);
                break;
            case StoreAttr:
                /* X Y -> */
                name = SbStr_AsString(SbTuple_GetItem(names, opcode_arg));
                op1 = STACK_POP();
                op2 = STACK_POP();
                i_result = SbObject_SetAttrString(op1, name, op2);
                Sb_DECREF(op2);
                goto XxxName_drop1_check_iresult;
            case DeleteAttr:
                /* X -> */
                name = SbStr_AsString(SbTuple_GetItem(names, opcode_arg));
                op1 = STACK_POP();
                i_result = SbObject_DelAttrString(op1, name);

XxxName_drop1_check_iresult:
                Sb_DECREF(op1);
XxxName_check_iresult:
                if (i_result >= 0) {
                    continue;
                }
                SbErr_Clear();
                SbErr_RaiseWithString(SbErr_AttributeError, name);
                break;


            case BuildTuple:
                o_result = SbTuple_New(opcode_arg);
                if (!o_result) {
                    /* NOTE: excessive args will be popped either on function exit or exception handling */
                    goto Xxx_check_error;
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
                    /* NOTE: excessive args will be popped either on function exit or exception handling */
                    goto Xxx_check_error;
                }
                pos = opcode_arg - 1;
                while (pos >= 0) {
                    tmp = STACK_POP();
                    SbList_SetItemUnsafe(o_result, pos, tmp);
                    --pos;
                }
                STACK_PUSH(o_result);
                continue;
            case BuildMap:
                /* Nothing is pushed on the stack. */
                o_result = SbDict_New();
                /* NOTE: the compiler provides a sizing hint via opcode arg, which is ignored for now. */
                goto Xxx_check_oresult;

            case StoreMap:
                /* Key Value Dict -> Dict */
                op1 = STACK_POP();
                op2 = STACK_POP();
                tmp = STACK_TOP();
                /* assert(SbDict_CheckExact(tmp)); */
                i_result = SbDict_SetItem(tmp, op1, op2);
                goto Xxx_drop2_check_iresult;

            case MakeFunction:
                /* C DefaultN DefaultN-1 ... -> F */
                op1 = STACK_POP();
                op2 = SbTuple_New(opcode_arg);
                if (!op2) {
                    /* NOTE: excessive args will be popped either on function exit or exception handling */
                    goto Xxx_check_error;
                }
                pos = opcode_arg - 1;
                while (pos >= 0) {
                    tmp = STACK_POP();
                    SbTuple_SetItemUnsafe(op2, pos, tmp);
                    --pos;
                }

                o_result = SbPFunction_New((SbCodeObject *)op1, op2, globals);
                goto Xxx_drop2_check_oresult;

            case CallFunction:
            case CallFunctionVar:
            case CallFunctionKw:
            case CallFunctionVarKw:
                {
                    Sb_ssize_t posargs_passed, kwargs_passed;
                    Sb_ssize_t total_posargs;
                    int argflags = opcode - CallFunction;
                    SbObject *vargs = NULL;
                    SbObject *kwds = NULL;

                    /* **KWA *VA KwArgs PosArgs */

                    if (argflags & 2) {
                        kwds = STACK_POP();
                        if (!SbDict_CheckExact(kwds)) {
                        }
                    }
                    if (argflags & 1) {
                        vargs = STACK_POP();
                        if (!SbList_CheckExact(vargs) && !SbTuple_CheckExact(vargs)) {
                        }
                    }

                    kwargs_passed = (opcode_arg >> 8) & 0xFF;
                    op3 = SbDict_New();
                    if (!op3) {
                        goto Xxx_check_error;
                    }
                    if (kwds) {
                        /* TODO: Verify that kwds contains only string keys */
                        i_result = SbDict_Merge(op3, kwds, 1);
                        Sb_DECREF(kwds);
                        if (i_result < 0) {
                            Sb_DECREF(op3);
                            goto Xxx_check_error;
                        }
                    }
                    if (kwargs_passed) {
                        /* On the stack, the opcode finds the keyword parameters first.
                        For each keyword argument, the value is on top of the key.
                        */
                        pos = 0;
                        while (pos < kwargs_passed) {
                            SbObject *key;
                            SbObject *value;

                            value = STACK_POP();
                            key = STACK_POP();
                            /* NOTE: This overrides whatever was passed via **kwds */
                            SbDict_SetItemString(op3, SbStr_AsString(key), value);
                            Sb_DECREF(key);
                            Sb_DECREF(value);
                            ++pos;
                        }
                    }

                    posargs_passed = opcode_arg & 0xFF;
                    total_posargs = posargs_passed;
                    if (vargs) {
                        total_posargs += SbSequence_GetSize(vargs);
                    }
                    op2 = SbTuple_New(total_posargs);
                    if (!op2) {
                        Sb_DECREF(op3);
                        goto Xxx_check_error;
                    }
                    /* Below the keyword parameters, the positional parameters 
                    are on the stack, with the right-most parameter on top.
                    */
                    if (posargs_passed) {
                        pos = posargs_passed - 1;
                        while (pos >= 0) {
                            SbTuple_SetItemUnsafe(op2, pos, STACK_POP());
                            --pos;
                        }
                    }
                    if (vargs) {
                        SbObject *value;

                        pos = posargs_passed;
                        while (pos < total_posargs) {
                            value = SbSequence_GetItem(vargs, pos);
                            SbTuple_SetItemUnsafe(op2, pos, value);
                            ++pos;
                        }
                    }

                    op1 = STACK_POP();

                    o_result = SbObject_Call(op1, op2, op3);
                    goto Xxx_drop3_check_oresult;
                }
                break;

            case ImportFrom:
                /* Mod -> Attr Mod */
                name = SbStr_AsString(SbTuple_GetItem(names, opcode_arg));
                /* assert(Sb_TYPE(STACK_TOP()) == SbModule_Type); */
                o_result = SbDict_GetItemString(SbModule_GetDict(STACK_TOP()), name);
                if (o_result) {
                    goto Xxx_incref_push_continue;
                }
                if (!SbErr_Occurred()) {
                    SbErr_RaiseWithString(SbErr_NameError, name);
                }
                break;

            case ImportName:
                /* FromList Level -> Mod */
                op1 = STACK_POP();
                op2 = STACK_POP();
                name = SbStr_AsString(SbTuple_GetItem(names, opcode_arg));
                o_result = SB_Import(name);
                goto Xxx_drop2_check_oresult;

            case ImportStar:
                /* Mod -> */
                op1 = STACK_POP();
                /* assert(Sb_TYPE(op1) == SbModule_Type); */
                i_result = SbDict_Merge(locals, SbModule_GetDict(op1), 1);
                /* TODO: how to resolve name conflicts? */
                goto Xxx_drop1_check_iresult;

            case UnaryNot:
                /* X -> (not X) */
                op1 = STACK_POP();
                i_result = SbObject_Not(op1);
                Sb_DECREF(op1);
                if (i_result < 0) {
                    goto Xxx_check_error;
                }
                STACK_PUSH(SbBool_FromLong(i_result));
                continue;
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
                op2 = STACK_POP();
                op1 = STACK_POP();

                if (opcode_arg <= SbCmp_GE) {
                    o_result = SbObject_Compare(op1, op2, (SbObjectCompareOp)opcode_arg);
                    goto Xxx_drop2_check_oresult;
                }
                if (opcode_arg == SbCmp_IS) {
                    o_result = SbBool_FromLong(op1 == op2);
                    goto Xxx_drop2_check_oresult;
                }
                if (opcode_arg == SbCmp_IS_NOT) {
                    o_result = SbBool_FromLong(op1 != op2);
                    goto Xxx_drop2_check_oresult;
                }
                if (opcode_arg == SbCmp_EXC_MATCH) {
                    i_result = SbErr_ExceptionTypeMatches((SbTypeObject *)op1, op2);
                    if (i_result < 0) {
                        goto Xxx_check_error;
                    }
                    o_result = SbBool_FromLong(i_result);
                    goto Xxx_drop2_check_oresult;
                }
                SbErr_RaiseWithFormat(SbErr_SystemError, "compare op %d not implemented", opcode_arg);
                break;

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

            case GetIter:
                /* X -> iter(X) */
                op1 = STACK_POP();
                o_result = SbObject_GetIter(op1);
                goto Xxx_drop1_check_oresult;

            case ForIter:
                op1 = STACK_TOP();
                o_result = SbIter_Next(op1);
                if (o_result) {
                    STACK_PUSH(o_result);
                    continue;
                }
                else {
                    ++sp;
                    Sb_DECREF(op1);
                    ip += opcode_arg;
                    continue;
                }


            case RaiseVarArgs:
                /* X Y Z -> */
                /* NOTE: raise Z, Y, X */
                if (opcode_arg > 0) {
                    /* Raise a new exception. */

                    op2 = NULL;
                    switch (opcode_arg) {
                    case 3:
                        /* TODO: tracebacks */
                        op3 = STACK_POP();
                        Sb_XDECREF(op3);
                        /* Fall through */
                    case 2:
                        op2 = STACK_POP();
                        /* Fall through */
                    case 1:
                        op1 = STACK_POP();
                        if (!SbType_Check(op1)) {
                            Sb_XDECREF(op2);
                            Sb_XDECREF(op1);
                            SbErr_RaiseWithString(SbErr_TypeError, "only type objects can be passed at 1st parameter to raise");
                            break;
                        }
                    }

                    SbErr_RaiseWithObject((SbTypeObject *)op1, op2);
                    Sb_XDECREF(op2);
                    Sb_XDECREF(op1);
                }
                else {
                    SbObject *current_exc = frame->current_exc;

                    /* Reraise the previous exception */
                    if (!current_exc) {
                        SbErr_RaiseWithString(SbErr_ValueError, "cannot reraise if no exception has been raised");
                        break;
                    }

                    SbErr_Restore(current_exc);
                }
                break;

            case SetupExcept:
            case SetupFinally:
                i_result = SbFrame_PushBlock(frame, ip + opcode_arg, sp, opcode);
                if (i_result < 0) {
                    goto Xxx_check_error;
                }
                continue;

            case EndFinally:
                op1 = STACK_POP();

                /* None -> */
                if (op1 == Sb_None) {
                    /* No exception occurred or it was handled */
                    Sb_DECREF(op1);
                    Sb_CLEAR(frame->current_exc);
                    continue;
                }

                /* Reason [RetVal] -> */
                if (SbInt_CheckExact(op1)) {
                    /* Restore unwind reason and go on with unwinding */
                    reason = (enum SbUnwindReason)SbInt_AsNativeUnsafe(op1);
                    Sb_DECREF(op1);
                    if (reason == Reason_Return) {
                        return_value = STACK_POP();
                    }
                    break;
                }

                /* reason = Reason_Error; */

                /* Type Instance Traceback -> */
                if (SbType_Check(op1)) {
                    Sb_DECREF(op1);
                    op2 = STACK_POP(); /* Value */
                    op3 = STACK_POP(); /* Traceback */
                    Sb_DECREF(op3);
                    Sb_CLEAR(frame->current_exc);
                    SbErr_Restore(op2);
                    break;
                }

                SbErr_RaiseWithString(SbErr_SystemError, "END_FINALLY found something odd on the stack");
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


            case BuildSlice:
                op3 = STACK_POP();
                op2 = STACK_POP();
                op1 = STACK_POP();
                o_result = SbSlice_New(op1, op2, op3);
                goto Xxx_drop3_check_oresult;

            case Slice:
                /* X -> X[:] */
                op3 = NULL;
                op2 = NULL;
                op1 = STACK_POP();
                goto SliceXxx;
            case Slice+1:
                /* X Y -> Y[X:] */
                op3 = NULL;
                op2 = STACK_POP();
                op1 = STACK_POP();
                goto SliceXxx;
            case Slice+2:
                /* X Y -> Y[:X] */
                op3 = STACK_POP();
                op2 = NULL;
                op1 = STACK_POP();
                goto SliceXxx;
            case Slice+3:
                /* X Y Z -> Z[Y:X] */
                op3 = STACK_POP();
                op2 = STACK_POP();
                op1 = STACK_POP();
SliceXxx:
                tmp = SbSlice_New(op2, op3, NULL);
                Sb_XDECREF(op3);
                Sb_XDECREF(op2);
                if (!tmp) {
                    goto Xxx_check_error;
                }
                op2 = tmp;
                o_result = SbObject_GetItem(op1, op2);
                goto Xxx_drop2_check_oresult;

            case StoreSlice:
                op4 = NULL;
                op3 = NULL;
                goto StoreSliceXxx;
            case StoreSlice+1:
                op4 = NULL;
                op3 = STACK_POP();
                goto StoreSliceXxx;
            case StoreSlice+2:
                op4 = STACK_POP();
                op3 = NULL;
                goto StoreSliceXxx;
            case StoreSlice+3:
                op4 = STACK_POP();
                op3 = STACK_POP();
StoreSliceXxx:
                op2 = STACK_POP();
                op1 = STACK_POP();
                /* op1 = val, op2 = obj, op3 = start, op4 = end */
                tmp = SbSlice_New(op3, op4, NULL);
                Sb_XDECREF(op4);
                Sb_XDECREF(op3);
                if (!tmp) {
                    goto Xxx_check_error;
                }
                op3 = tmp;
                i_result = SbObject_SetItem(op2, op1, op3);
                goto Xxx_drop3_check_iresult;

            case DeleteSlice:
                /* X -> */
                op3 = NULL;
                op2 = NULL;
                op1 = STACK_POP();
                goto DeleteSliceXxx;
            case DeleteSlice+1:
                /* X Y -> */
                op3 = NULL;
                op2 = STACK_POP();
                op1 = STACK_POP();
                goto DeleteSliceXxx;
            case DeleteSlice+2:
                /* X Y -> */
                op3 = STACK_POP();
                op2 = NULL;
                op1 = STACK_POP();
                goto DeleteSliceXxx;
            case DeleteSlice+3:
                /* X Y Z -> */
                op3 = STACK_POP();
                op2 = STACK_POP();
                op1 = STACK_POP();
DeleteSliceXxx:
                tmp = SbSlice_New(op2, op3, NULL);
                Sb_XDECREF(op3);
                Sb_XDECREF(op2);
                if (!tmp) {
                    goto Xxx_check_error;
                }
                op2 = tmp;
                i_result = SbObject_DelItem(op1, op2);
                goto Xxx_drop2_check_iresult;


            case BuildClass:
                /* PropsDict Bases Name -> Type */
                op3 = STACK_POP();
                op2 = STACK_POP();
                op1 = STACK_POP();
                o_result = _SbType_New(op1, op2, op3);
                goto Xxx_drop3_check_oresult;


            default:
                /* Not implemented. */
                SbErr_RaiseWithFormat(SbErr_SystemError, "opcode %d not implemented", opcode);
                break;

Xxx_drop3_check_oresult:
                Sb_DECREF(op3);
Xxx_drop2_check_oresult:
                Sb_DECREF(op2);
Xxx_drop1_check_oresult:
                Sb_DECREF(op1);
Xxx_check_oresult:
                if (o_result) {
                    STACK_PUSH(o_result);
                    continue;
                }
                goto Xxx_check_error;

Xxx_incref_push_continue:
                Sb_INCREF(o_result);
                STACK_PUSH(o_result);
                continue;

Xxx_drop3_check_iresult:
                Sb_DECREF(op3);
Xxx_drop2_check_iresult:
                Sb_DECREF(op2);
Xxx_drop1_check_iresult:
                Sb_DECREF(op1);
                if (!i_result) {
                    continue;
                }
Xxx_check_error:
                if (!SbErr_Occurred()) {
                    SbErr_RaiseWithString(SbErr_SystemError, "call result is NULL or -1 but no error is set");
                }
                break;
            } /* switch (opcode) */
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
                SbObject *tmp;

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
                if (reason == Reason_Error) {
                    SbObject *tmp;
                    SbObject *exc;

                    /* The frame now owns references to exception info. */
                    SbErr_Fetch(&exc);
                    frame->current_exc = exc;

                    /* Both `finally` and `except`: Type Instance TraceBack -> */
                    tmp = Sb_None;
                    Sb_INCREF(tmp);
                    STACK_PUSH(tmp);
                    Sb_INCREF(exc);
                    STACK_PUSH(exc);
                    tmp = (SbObject *)Sb_TYPE(exc);
                    Sb_INCREF(tmp);
                    STACK_PUSH(tmp);
                }
                else {
                    if (reason == Reason_Return) {
                        STACK_PUSH(return_value);
                        return_value = NULL;
                    }

                    STACK_PUSH(SbInt_FromNative(reason));
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

