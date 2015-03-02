#include "snakebed.h"

SbObject *Sb_ModuleSys = NULL;

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
_Sb_ModuleInit_Sys()
{
    SbObject *m;
    SbObject *dict;
    SbObject *o;

    m = Sb_InitModule("sys");
    if (!m) {
        return -1;
    }

    dict = SbModule_GetDict(m);
    if (!dict) {
        return -1;
    }

    o = SbFile_FromHandle(Sb_GetStdInHandle());
    SbDict_SetItemString(dict, "stdin", o);
    o = SbFile_FromHandle(Sb_GetStdOutHandle());
    SbDict_SetItemString(dict, "stdout", o);

    Sb_ModuleSys = m;
    return 0;
}

