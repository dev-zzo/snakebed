#include "snakebed.h"

SbObject *Sb_ModuleBuiltin = NULL;

#if SUPPORTS_BUILTIN_PRINT
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
#endif

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

#if SUPPORTS_BUILTIN_PRINT
    add_func(dict, "print", _builtin_print);
#endif

    Sb_ModuleBuiltin = m;
    return 0;
}

