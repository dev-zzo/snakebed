#include "snakebed.h"
#include "builtin.h"
#include "sys.h"

SbObject *Sb_ModuleBuiltin = NULL;

#if SUPPORTS(BUILTIN_PRINT)
static SbObject *
_builtin_print(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *out;
    SbObject *sep;
    SbObject *end;

    /* TODO: these are not needed when taken from args. */
    sep = SbStr_FromString(" ");
    end = SbStr_FromString("\n");

    out = SbDict_GetItemString(SbModule_GetDict(Sb_ModuleSys), "stdout");
    if (out) {
        Sb_ssize_t pos, count;
        
        count = SbTuple_GetSizeUnsafe(args);
        for (pos = 0; pos < count; ++pos) {
            SbObject *o;
            SbObject *stro;

            o = SbTuple_GetItemUnsafe(args, pos);
            stro = SbObject_Str(o);
            if (!stro) {
                break;
            }

            SbFile_Write(out, SbStr_AsStringUnsafe(stro), SbStr_GetSizeUnsafe(stro));
            Sb_DECREF(stro);
            SbFile_Write(out, SbStr_AsStringUnsafe(sep), SbStr_GetSizeUnsafe(sep));
        }

        SbFile_Write(out, SbStr_AsStringUnsafe(end), SbStr_GetSizeUnsafe(end));
    }

    /* TODO: these are not needed when taken from args. */
    Sb_DECREF(sep);
    Sb_DECREF(end);
    Sb_RETURN_NONE;
}
#endif /* SUPPORTS(BUILTIN_PRINT) */

static SbObject *
_builtin_id(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o;

    if (SbArgs_Unpack(args, 1, 1, &o) < 0) {
        return NULL;
    }

    return SbInt_FromNative((SbInt_Native_t)o);
}

static SbObject *
_builtin_len(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o;
    Sb_ssize_t len;

    if (SbArgs_Unpack(args, 1, 1, &o) < 0) {
        return NULL;
    }

    len = SbObject_GetSize(o);
    if (len == -1) {
        return NULL;
    }

    return SbInt_FromNative(len);
}

static SbObject *
_builtin_hash(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o;
    SbInt_Native_t hash;

    if (SbArgs_Unpack(args, 1, 1, &o) < 0) {
        return NULL;
    }

    hash = SbObject_Hash(o);
    if (hash == -1) {
        return NULL;
    }

    return SbInt_FromNative(hash);
}


static SbObject *
_builtin_getattr(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o;
    SbObject *o_name;
    SbObject *o_default = NULL;

    if (SbArgs_Unpack(args, 2, 3, &o, &o_name, &o_default) < 0) {
        return NULL;
    }
    if (!SbStr_CheckExact(o_name)) {
        SbErr_RaiseWithString(SbExc_TypeError, "attribute name must be a string");
        return NULL;
    }

    o = SbObject_GetAttrString(o, SbStr_AsStringUnsafe(o_name));
    if (o) {
        return o;
    }
    if (SbErr_Occurred()) {
        return NULL;
    }
    if (o_default) {
        Sb_INCREF(o_default);
        return o_default;
    }
    SbErr_RaiseWithObject(SbExc_AttributeError, o_name);
    return NULL;
}

SbObject *
SbBuiltin_Format(SbObject *self, SbObject *spec)
{
    return SbObject_CallMethodObjArgs(self, "__format__", 1, spec);
}

static SbObject *
_builtin_format(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *spec;

    if (SbArgs_Unpack(args, 1, 1, &spec) < 0) {
        return NULL;
    }

    return SbObject_CallMethodObjArgs(self, "__format__", 1, spec);
}

static int
add_func(SbObject *dict, const char *name, SbCFunction func)
{
    SbObject *f;

    f = SbCFunction_New(func);
    if (!f) {
        return -1;
    }
    if (SbDict_SetItemString(dict, name, f) < 0) {
        Sb_DECREF(f);
        return -1;
    }
    return 0;
}

int
_Sb_ModuleInit_Builtin()
{
    SbObject *m;
    SbObject *dict;

    m = Sb_InitModule("__builtin__");
    if (!m) {
        return -1;
    }

    dict = SbModule_GetDict(m);
    if (!dict) {
        return -1;
    }

    SbDict_SetItemString(dict, "BaseException", (SbObject *)SbExc_BaseException);
    SbDict_SetItemString(dict, "Exception", (SbObject *)SbExc_Exception);
    SbDict_SetItemString(dict, "StandardError", (SbObject *)SbExc_StandardError);
    SbDict_SetItemString(dict, "AttributeError", (SbObject *)SbExc_AttributeError);
    SbDict_SetItemString(dict, "EnvironmentError", (SbObject *)SbExc_EnvironmentError);
    SbDict_SetItemString(dict, "IOError", (SbObject *)SbExc_IOError);
    SbDict_SetItemString(dict, "ImportError", (SbObject *)SbExc_ImportError);
    SbDict_SetItemString(dict, "LookupError", (SbObject *)SbExc_LookupError);
    SbDict_SetItemString(dict, "IndexError", (SbObject *)SbExc_IndexError);
    SbDict_SetItemString(dict, "KeyError", (SbObject *)SbExc_KeyError);
    SbDict_SetItemString(dict, "MemoryError", (SbObject *)SbExc_MemoryError);
    SbDict_SetItemString(dict, "NameError", (SbObject *)SbExc_NameError);
    SbDict_SetItemString(dict, "UnboundLocalError", (SbObject *)SbExc_UnboundLocalError);
    SbDict_SetItemString(dict, "SystemError", (SbObject *)SbExc_SystemError);
    SbDict_SetItemString(dict, "TypeError", (SbObject *)SbExc_TypeError);
    SbDict_SetItemString(dict, "ValueError", (SbObject *)SbExc_ValueError);
    SbDict_SetItemString(dict, "StopIteration", (SbObject *)SbExc_StopIteration);
    SbDict_SetItemString(dict, "KeyboardInterrupt", (SbObject *)SbExc_KeyboardInterrupt);

    SbDict_SetItemString(dict, "object", (SbObject *)SbObject_Type);
    SbDict_SetItemString(dict, "type", (SbObject *)SbType_Type);
    SbDict_SetItemString(dict, "int", (SbObject *)SbInt_Type);
    SbDict_SetItemString(dict, "str", (SbObject *)SbStr_Type);
    SbDict_SetItemString(dict, "tuple", (SbObject *)SbTuple_Type);
    SbDict_SetItemString(dict, "list", (SbObject *)SbList_Type);
    SbDict_SetItemString(dict, "dict", (SbObject *)SbDict_Type);
    SbDict_SetItemString(dict, "iter", (SbObject *)SbIter_Type);

    SbDict_SetItemString(dict, "True", Sb_True);
    SbDict_SetItemString(dict, "False", Sb_False);
    SbDict_SetItemString(dict, "None", Sb_None);
    SbDict_SetItemString(dict, "NotImplemented", Sb_NotImplemented);

#if SUPPORTS(BUILTIN_PRINT)
    add_func(dict, "print", _builtin_print);
#endif
    add_func(dict, "id", _builtin_id);
    add_func(dict, "len", _builtin_len);
    add_func(dict, "hash", _builtin_hash);
    add_func(dict, "getattr", _builtin_getattr);
#if SUPPORTS(STR_FORMAT)
    add_func(dict, "format", _builtin_format);
#endif

    Sb_ModuleBuiltin = m;
    return 0;
}

void
_Sb_ModuleFini_Builtin()
{
    Sb_CLEAR(Sb_ModuleBuiltin);
}
