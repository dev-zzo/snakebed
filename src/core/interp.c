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
    SbObject *tmp;

    /* Link the new frame into frame chain. */
    SbFrame_SetPrevious((SbObject *)frame, SbInterp_TopFrame);
    SbInterp_TopFrame = frame;
    code = frame->code;

    /* Setup initial values for sp and ip. */
    sp = frame->sp;
    ip = frame->ip;

    /* Loop until a return is executed or an exception is raised. */
    for (;;) {
        Sb_byte_t opcode;
        unsigned opcode_arg;

        opcode = *ip++;
        if (opcode >= HaveArgument) {
            opcode_arg = ip[0];
            opcode_arg = (opcode_arg << 8) | ip[1];
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
            tmp = *sp++;
            Sb_DECREF(tmp);
            /* No need for exception checks. */
            continue;

        case DupTop:
            /* X -> X X */
            tmp = *sp;
            Sb_INCREF(tmp);
            *--sp = tmp;
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
            tmp = SbTuple_GetItemUnsafe(code->consts, opcode_arg);
            Sb_INCREF(tmp);
            *--sp = tmp;
            /* No need for exception checks. */
            continue;

        case LoadLocals:
            tmp = frame->locals;
            Sb_INCREF(tmp);
            *--sp = tmp;
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
            return_value = *sp++;
            /* TODO: handle. */
            break;

            /* Always use global namespace. */
        case LoadGlobal:
            {
                SbObject *name;
                SbObject *o;

                name = SbTuple_GetItem(code->names, opcode_arg);
                if (frame->globals) {
                    o = SbDict_GetItemString(frame->globals, SbStr_AsStringUnsafe(name));
                }
                if (!o) {
                    /* Try builtins */
                }

                if (!o) {
                    SbErr_RaiseWithObject(SbErr_NameError, name);
                }
                else {
                    Sb_INCREF(o);
                    *--sp = o;
                }
            }
            break;
        case StoreGlobal:
            {
                SbObject *name;
                SbObject *o;

                name = SbTuple_GetItem(code->names, opcode_arg);
                o = *sp++;
                if (SbDict_SetItemString(frame->globals, SbStr_AsStringUnsafe(name), o) < 0) {
                    SbErr_Clear();
                    SbErr_RaiseWithObject(SbErr_NameError, name);
                }
                Sb_DECREF(o);
            }
            break;
        case DeleteGlobal:
            {
                SbObject *name;

                name = SbTuple_GetItem(code->names, opcode_arg);
                if (SbDict_DelItemString(frame->globals, SbStr_AsStringUnsafe(name)) < 0) {
                    SbErr_Clear();
                    SbErr_RaiseWithObject(SbErr_NameError, name);
                }
            }
            break;

            /* Use local, then global namespace */

        case LoadName:
        case LoadFast:
            {
                SbObject *name;
                SbObject *o = NULL;

                name = SbTuple_GetItem(opcode == LoadName ? code->names : code->fastnames, opcode_arg);
                if (frame->locals) {
                    o = SbDict_GetItemString(frame->locals, SbStr_AsStringUnsafe(name));
                }
                if (!o && frame->globals) {
                    o = SbDict_GetItemString(frame->globals, SbStr_AsStringUnsafe(name));
                }
                if (!o) {
                    SbErr_RaiseWithObject(SbErr_NameError, name);
                }
                else {
                    Sb_INCREF(o);
                    *--sp = o;
                }
            }
            break;
        case StoreName:
        case StoreFast:
            {
                SbObject *name;
                SbObject *o;

                name = SbTuple_GetItem(opcode == StoreName ? code->names : code->fastnames, opcode_arg);
                o = *sp++;
                if (SbDict_SetItemString(frame->locals, SbStr_AsStringUnsafe(name), o) < 0) {
                    /* No handling. */
                }
                Sb_DECREF(o);
            }
            break;
        case DeleteName:
        case DeleteFast:
            {
                SbObject *name;

                name = SbTuple_GetItem(opcode == DeleteName ? code->names : code->fastnames, opcode_arg);
                if (SbDict_DelItemString(frame->locals, SbStr_AsStringUnsafe(name)) < 0) {
                    if (SbErr_Occurred() && SbErr_ExceptionMatches(SbErr_Occurred(), (SbObject *)SbErr_KeyError)) {
                        SbErr_Clear();
                        if (SbDict_DelItemString(frame->globals, SbStr_AsStringUnsafe(name)) < 0) {
                            SbErr_Clear();
                            SbErr_RaiseWithObject(SbErr_NameError, name);
                        }
                    }
                }
            }
            break;

        case LoadAttr:
            /* X -> X.attr */
            {
                SbObject *attr_name;
                SbObject *attr;

                attr_name = SbTuple_GetItem(code->names, opcode_arg);
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

                attr_name = SbTuple_GetItem(code->names, opcode_arg);
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

                attr_name = SbTuple_GetItem(code->names, opcode_arg);
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

        case JumpIfFalseOrPop:
        case JumpIfTrueOrPop:
        case PopJumpIfFalse:
        case PopJumpIfTrue:
            break;

        case MakeFunction:
            {
                SbObject *code;
                SbObject *defaults;
                SbObject *func;
                Sb_ssize_t pos;

                code = *sp++;

                defaults = SbTuple_New(opcode_arg);
                for (pos = 0; pos < (Sb_ssize_t)opcode_arg; ++pos) {
                    SbTuple_SetItemUnsafe(defaults, pos, sp[pos]);
                }
                sp += opcode_arg;

                func = SbPFunction_New((SbCodeObject *)code, defaults, frame->globals);
                Sb_DECREF(code);
                if (func) {
                    *--sp = func;
                }
            }
            break;

        case CallFunction:
            {
                Sb_ssize_t posargs_passed, kwargs_passed;
                SbObject *posargs_tuple = NULL;
                SbObject *kwargs_dict = NULL;
                SbObject *callable;
                SbObject *result;
                Sb_ssize_t pos;

                posargs_passed = opcode_arg & 0xFF;
                kwargs_passed = (opcode_arg >> 8) & 0xFF;

#if SUPPORTS_KWARGS
                /* On the stack, the opcode finds the keyword parameters first.
                For each keyword argument, the value is on top of the key.
                */
                kwargs_dict = SbDict_New();
                if (!kwargs_dict) {
                    break;
                }
                pos = 0;
                while (pos < kwargs_passed) {
                    SbObject *key;
                    SbObject *value;

                    key = *sp++;
                    value = *sp++;
                    SbDict_SetItemString(kwargs_dict, SbStr_AsString(key), value);
                    Sb_DECREF(key);
                    Sb_DECREF(value);
                    ++pos;
                }
#else
                if (kwargs_passed) {
                    /* Whine terribly */
                }
#endif
                /* Below the keyword parameters, the positional parameters 
                are on the stack, with the right-most parameter on top.
                */
                posargs_tuple = SbTuple_New(posargs_passed);
                if (!posargs_tuple) {
                    break;
                }
                pos = posargs_passed - 1;
                while (pos >= 0) {
                    SbTuple_SetItemUnsafe(posargs_tuple, pos, *sp++);
                    --pos;
                }

                callable = *sp++;

                result = SbObject_Call(callable, posargs_tuple, kwargs_dict);
                if (result) {
                    *--sp = result;
                }

                Sb_DECREF(callable);
                Sb_DECREF(posargs_tuple);
#if SUPPORTS_KWARGS
                Sb_DECREF(kwargs_dict);
#endif
            }
            break;

        default:
            /* Not implemented. */
            break;
        }

    }

    return return_value;
}

