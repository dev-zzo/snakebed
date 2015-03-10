#include "snakebed.h"

SbObject *Sb_ModuleSys = NULL;

static void
set_item_or_none(SbObject *tuple, Sb_ssize_t pos, SbObject *o)
{
    SbObject *tmp;

    if (o) {
        tmp = o;
    }
    else {
        tmp = Sb_None;
        Sb_INCREF(tmp);
    }
    SbTuple_SetItemUnsafe(tuple, pos, tmp);
}

static SbObject *
exc_info(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *result;
    SbExceptionInfo* info;

    result = SbTuple_New(3);
    if (!result) {
        return NULL;
    }

    info = &SbInterp_TopFrame->exc_info;
    set_item_or_none(result, 0, (SbObject *)info->type);
    set_item_or_none(result, 1, (SbObject *)info->value);
    set_item_or_none(result, 2, (SbObject *)info->traceback);

    return result;
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

    add_func(dict, "exc_info", exc_info);

    Sb_ModuleSys = m;
    return 0;
}

