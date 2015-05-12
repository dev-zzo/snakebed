#include "snakebed.h"

SbObject *Sb_ModuleSys = NULL;
SbObject *SbSys_Modules = NULL;
SbObject *SbSys_StdIn = NULL;
SbObject *SbSys_StdOut = NULL;
SbObject *SbSys_StdErr = NULL;

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

static SbObject *
_sys_exit(SbObject *self, SbObject *args, SbObject *kwargs)
{
    /* TODO: handle the passed arg. */
    SbErr_RaiseWithObject(SbErr_SystemExit, NULL);
    return NULL;
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

    /* Needs to be available beforehand. */
    o = SbDict_New();
    if (!o) {
        return -1;
    }
    SbSys_Modules = o;

    m = Sb_InitModule("sys");
    if (!m) {
        return -1;
    }

    dict = SbModule_GetDict(m);
    if (!dict) {
        return -1;
    }

    o = SbFile_FromHandle(Sb_GetStdInHandle());
    if (!o) {
        return -1;
    }
    SbDict_SetItemString(dict, "stdin", o);
    Sb_DECREF(o);
    SbSys_StdIn = o;
    o = SbFile_FromHandle(Sb_GetStdOutHandle());
    if (!o) {
        return -1;
    }
    SbDict_SetItemString(dict, "stdout", o);
    Sb_DECREF(o);
    SbSys_StdOut = o;
    o = SbFile_FromHandle(Sb_GetStdErrHandle());
    if (!o) {
        return -1;
    }
    SbDict_SetItemString(dict, "stderr", o);
    Sb_DECREF(o);
    SbSys_StdErr = o;

    SbDict_SetItemString(dict, "modules", SbSys_Modules);
    Sb_DECREF(SbSys_Modules);

    add_func(dict, "exc_info", exc_info);
    add_func(dict, "exit", _sys_exit);

    Sb_ModuleSys = m;
    return 0;
}


void
_Sb_ModuleFini_Sys()
{
    SbObject *dict;

    dict = SbModule_GetDict(Sb_ModuleSys);
    SbDict_DelItemString(dict, "modules");
    Sb_CLEAR(Sb_ModuleSys);
    SbSys_StdIn = NULL;
    SbSys_StdOut = NULL;
    SbSys_StdErr = NULL;
}
