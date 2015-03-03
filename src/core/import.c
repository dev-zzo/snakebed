#include "snakebed.h"

SbObject *Sb_LoadedModules = NULL;

SbObject *
Sb_ReadObjectFromPath(const char *path);

SbObject *
Sb_InitModule(const char *name)
{
    SbObject *m;

    m = SbModule_New(name);
    if (!m) {
        return NULL;
    }

    if (!Sb_LoadedModules) {
        Sb_LoadedModules = SbDict_New();
        if (!Sb_LoadedModules) {
            Sb_DECREF(m);
            return NULL;
        }
    }

    SbDict_SetItemString(Sb_LoadedModules, name, m);
    Sb_DECREF(m);

    return m;
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
    if (Sb_LoadedModules) {
        SbDict_DelItemString(Sb_LoadedModules, name);
    }
    Sb_DECREF(module);
fail0:
    return NULL;
}

