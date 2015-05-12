#include "snakebed.h"

SbObject *
Sb_ReadObjectFromPath(const char *path);

SbObject *
Sb_InitModule(const char *name)
{
    SbObject *modules = SbSys_Modules;
    SbObject *module;

    module = SbModule_New(name);
    if (!module) {
        return NULL;
    }

    SbDict_SetItemString(modules, name, module);
    /* Sb_DECREF(module); */

    return module;
}

SbObject *
Sb_GetModule(const char *name)
{
    SbObject *modules = SbSys_Modules;

    if (!modules) {
        return NULL;
    }

    return SbDict_GetItemString(modules, name);
}

SbObject *
Sb_LoadModule(const char *name, const char *path)
{
    SbCodeObject *module_code;
    SbObject *module;
    SbObject *module_func;
    SbObject *call_result;

    module = Sb_InitModule(name);
    if (!module) {
        goto fail0;
    }

    module_code = (SbCodeObject *)Sb_ReadObjectFromPath(path);
    if (!module_code) {
        goto fail1;
    }
    if (!SbCode_Check(module_code)) {
        Sb_DECREF(module_code);
        SbErr_RaiseWithString(SbErr_ValueError, "the loaded object is not code");
        goto fail1;
    }

    module_func = SbPFunction_New(module_code, SbTuple_New(0), SbObject_DICT(module));
    Sb_DECREF(module_code);
    if (!module_func) {
        goto fail1;
    }

    call_result = SbPFunction_Call(module_func, NULL, NULL);
    Sb_DECREF(module_func);
    if (!call_result) {
        goto fail1;
    }
    Sb_DECREF(call_result);

    return module;

fail1:
    SbDict_DelItemString(SbSys_Modules, name);
    Sb_DECREF(module);
fail0:
    return NULL;
}

void
_Sb_UnloadModule(const char *name)
{
    SbDict_DelItemString(SbSys_Modules, name);
}

SbObject *
SB_Import(const char *name)
{
    SbObject *module;

    module = Sb_GetModule(name);
    if (module) {
        Sb_INCREF(module);
        return module;
    }

    /* NOTE: Temporary until I implement some package support. */
    SbErr_RaiseWithFormat(SbErr_ImportError, "no module named '%s'", name);
    return NULL;
}
