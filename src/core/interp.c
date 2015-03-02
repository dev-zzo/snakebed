#include "snakebed.h"
#include "interp.h"
#include "opcode.h"

SbFrameObject *SbInterp_TopFrame = NULL;

SbObject *
SbInterp_Execute(SbFrameObject *frame)
{
    SbObject *return_value = NULL;
    SbCodeObject *code;
    const Sb_byte_t *ip;
    SbObject **sp;

    /* Link the new frame into frame chain. */
    SbFrame_SetPrevious((SbObject *)frame, SbInterp_TopFrame);
    SbInterp_TopFrame = frame;
    code = frame->code;

    /* Setup initial values for sp and ip. */
    sp = frame->sp;
    ip = frame->ip;

    /* Loop until a return is executed or an exception is raised. */
    for (;;) {
        SbOpcode opcode;
        unsigned opcode_arg;
        SbObject *tmp;
        SbObject *op1, *op2, *op3;
        SbObject *scope;
        SbObject *name;
        const char *method_name;
        SbObject *result;
        int i_result;
        int test_value;
        Sb_ssize_t pos;
        SbUnaryFunc ufunc;
        SbBinaryFunc bfunc;
        int failure = 0;

        opcode = (SbOpcode)(*ip++);
        if (opcode >= HaveArgument) {
            opcode_arg = ip[0];
            opcode_arg |= ip[1] << 8;
            ip += 2;
        }

        switch (opcode) {

            /*** Opcodes that never raise an exception ***/

        case Nop:
            /* No operation. */
            /* No need for exception checks. */
            continue;

        case PopTop:
            /* X -> */
            op1 = *sp++;
            Sb_DECREF(op1);
            /* No need for exception checks. */
            continue;

        case DupTop:
            /* X -> X X */
            op1 = *sp;
            Sb_INCREF(op1);
            *--sp = op1;
            /* No need for exception checks. */
            continue;

        case RotTwo:
            /* X Y -> Y X */
            tmp = sp[0];
            sp[0] = sp[1];
            sp[1] = tmp;
            /* No need for exception checks. */
            continue;
        case RotThree:
            /* X Y Z -> Y Z X */
            tmp = sp[0];
            sp[0] = sp[1];
            sp[1] = sp[2];
            sp[2] = tmp;
            /* No need for exception checks. */
            continue;
        case RotFour:
            /* X Y Z W -> Y Z W X */
            tmp = sp[0];
            sp[0] = sp[1];
            sp[1] = sp[2];
            sp[2] = sp[3];
            sp[3] = tmp;
            /* No need for exception checks. */
            continue;

        case LoadConst:
            result = SbTuple_GetItemUnsafe(code->consts, opcode_arg);
            Sb_INCREF(result);
            *--sp = result;
            /* No need for exception checks. */
            continue;

        case LoadLocals:
            result = frame->locals;
            Sb_INCREF(result);
            *--sp = result;
            /* No need for exception checks. */
            continue;

        case JumpForward:
            ip += opcode_arg;
            /* No need for exception checks. */
            continue;

        case JumpAbsolute:
            ip = SbStr_AsStringUnsafe(code->code) + opcode_arg;
            /* No need for exception checks. */
            continue;

            /*** Opcodes that may raise an exception ***/

        case JumpIfFalseOrPop:
            test_value = 0;
            goto JumpIfXxxOrPop;
        case JumpIfTrueOrPop:
            test_value = 1;
JumpIfXxxOrPop:
            op1 = *sp;
            i_result = SbObject_IsTrue(op1);
            failure = i_result < 0;
            if (failure) {
                break;
            }
            if (i_result == test_value) {
                ip = SbStr_AsStringUnsafe(code->code) + opcode_arg;
            }
            else {
                --sp;
                Sb_DECREF(op1);
            }
            break;

        case PopJumpIfFalse:
            test_value = 0;
            goto PopJumpIfXxx;
        case PopJumpIfTrue:
            test_value = 1;
PopJumpIfXxx:
            op1 = *sp++;
            i_result = SbObject_IsTrue(op1);
            failure = i_result < 0;
            Sb_DECREF(op1);
            if (failure) {
                break;
            }
            if (i_result == test_value) {
                ip = SbStr_AsStringUnsafe(code->code) + opcode_arg;
            }
            break;

        case LoadFast:
            if (!frame->locals) {
                failure = 1;
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
            }
LoadXxx_common:
            if (!result) {
                SbErr_RaiseWithObject(SbErr_NameError, name);
                failure = 1;
            }
            else {
                Sb_INCREF(result);
                *--sp = result;
            }
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
            op1 = *sp++;
            failure = SbDict_SetItemString(scope, SbStr_AsStringUnsafe(name), op1) < 0;
            Sb_DECREF(op1);
            if (failure) {
                SbErr_Clear();
                SbErr_RaiseWithObject(SbErr_NameError, name);
            }
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
                break;
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
            if (failure) {
                SbErr_Clear();
                SbErr_RaiseWithObject(SbErr_NameError, name);
            }
            break;

        case LoadAttr:
            /* X -> X.attr */
            name = SbTuple_GetItem(code->names, opcode_arg);
            op1 = *sp++;
            result = SbObject_GetAttrString(op1, SbStr_AsStringUnsafe(name));
            failure = result == NULL;
            Sb_DECREF(op1);
            if (failure) {
                SbErr_RaiseWithObject(SbErr_AttributeError, name);
            }
            else {
                *--sp = result;
            }
            break;
        case StoreAttr:
            /* X Y -> */
            name = SbTuple_GetItem(code->names, opcode_arg);
            op1 = *sp++;
            op2 = *sp++;
            failure = SbObject_SetAttrString(op1, SbStr_AsStringUnsafe(name), op2) < 0;
            Sb_DECREF(op1);
            Sb_DECREF(op2);
            break;
        case DeleteAttr:
            /* X -> */
            name = SbTuple_GetItem(code->names, opcode_arg);
            op1 = *sp++;
            failure = SbObject_DelAttrString(op1, SbStr_AsStringUnsafe(name)) < 0;
            Sb_DECREF(op1);
            break;

        case BuildTuple:
            result = SbTuple_New(opcode_arg);
            failure = result == NULL;
            if (failure) {
                goto BuildXxx_popargs;
            }
            pos = opcode_arg - 1;
            while (pos >= 0) {
                tmp = *sp++;
                SbTuple_SetItemUnsafe(result, pos, tmp);
                --pos;
            }
            *--sp = result;
            break;
        case BuildList:
            result = SbList_New(opcode_arg);
            failure = result == NULL;
            if (!failure) {
                pos = opcode_arg - 1;
                while (pos >= 0) {
                    tmp = *sp++;
                    SbList_SetItemUnsafe(result, pos, tmp);
                    --pos;
                }
                *--sp = result;
                break;
            }
BuildXxx_popargs:
            while (opcode_arg--) {
                tmp = *sp++;
                Sb_DECREF(tmp);
            }
            break;

        case MakeFunction:
            /* C DN DN-1 ... -> F */
            op1 = *sp++;
            op2 = SbTuple_New(opcode_arg);
            failure = op2 == NULL;
            if (failure) {
                goto BuildXxx_popargs;
            }

            pos = opcode_arg - 1;
            while (pos >= 0) {
                tmp = *sp++;
                SbTuple_SetItemUnsafe(op2, pos, tmp);
                --pos;
            }

            result = SbPFunction_New((SbCodeObject *)op1, op2, frame->globals);
            Sb_DECREF(op2);
            Sb_DECREF(op1);
            failure = result == NULL;
            if (!failure) {
                *--sp = result;
            }
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
                failure = op3 == NULL;
                if (failure) {
                    break;
                }
                pos = 0;
                while (pos < kwargs_passed) {
                    SbObject *key;
                    SbObject *value;

                    value = *sp++;
                    key = *sp++;
                    SbDict_SetItemString(op3, SbStr_AsString(key), value);
                    Sb_DECREF(key);
                    Sb_DECREF(value);
                    ++pos;
                }
#else
                if (kwargs_passed) {
                    /* Whine terribly */
                    failure = 1;
                    break;
                }
#endif
                /* Below the keyword parameters, the positional parameters 
                are on the stack, with the right-most parameter on top.
                */
                posargs_passed = opcode_arg & 0xFF;
                op2 = SbTuple_New(posargs_passed);
                failure = op2 == NULL;
                if (failure) {
#if SUPPORTS_KWARGS
                    Sb_DECREF(op3);
#endif
                    break;
                }
                pos = posargs_passed - 1;
                while (pos >= 0) {
                    SbTuple_SetItemUnsafe(op2, pos, *sp++);
                    --pos;
                }

                op1 = *sp++;

                result = SbObject_Call(op1, op2, op3);
#if SUPPORTS_KWARGS
                Sb_DECREF(op3);
#endif
                Sb_DECREF(op2);
                Sb_DECREF(op1);
                failure = result == NULL;
                if (!failure) {
                    *--sp = result;
                }
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
            op1 = *sp++;
            result = ufunc(op1);
            Sb_DECREF(op1);
            failure = result == NULL;
            if (!failure) {
                *--sp = result;
            }
            break;

        case CompareOp:
            /* X Y -> Y.__op__(X) */
            op1 = *sp++;
            op2 = *sp++;
            result = SbObject_Compare(op2, op1, opcode_arg);
BinaryXxx_tests:
            Sb_DECREF(op2);
            Sb_DECREF(op1);
            failure = result == NULL;
            if (!failure) {
                *--sp = result;
            }
            break;

        case InPlaceAdd:
        case BinaryAdd:
            bfunc = &SbNumber_Add;
BinaryXxx_common:
            op1 = *sp++;
            op2 = *sp++;
            result = bfunc(op2, op1);
            goto BinaryXxx_tests;
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

        case InPlacePower:
        case BinaryPower:

        case BinarySubscript:

        case BuildMap:
        case BuildSet:
        case UnaryNot:
        case UnaryConvert:
        case ReturnValue:
        default:
            /* Not implemented. */
            failure = 1;
            break;
        }

        if (failure) {

        }
    }

    return return_value;
}

