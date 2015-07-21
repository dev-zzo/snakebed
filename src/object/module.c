#include "snakebed.h"
#include "internal.h"

/* Keep the type object here. */
SbTypeObject *SbModule_Type = NULL;

SbObject *
SbModule_New(const char *name)
{
    SbModuleObject *myself;
    SbObject *name_str;
    SbObject *dict;

    myself = (SbModuleObject *)SbObject_New(SbModule_Type);
    if (!myself) {
        goto fail0;
    }

    name_str = SbStr_FromString(name);
    if (!name_str) {
        goto fail1;
    }

    SbObject_DICT(myself) = dict = SbDict_New();
    if (SbDict_SetItemString(dict, "__name__", name_str) < 0) {
        goto fail2;
    }
    Sb_DECREF(name_str);

    return (SbObject *)myself;

fail2:
    Sb_DECREF(name_str);
fail1:
    Sb_DECREF(myself);
fail0:
    return NULL;
}

static void
module_destroy(SbModuleObject *self)
{
    SbObject_DefaultDestroy((SbObject *)self);
}

/* Type initializer */

static const SbCMethodDef module_methods[] = {
    { "__getattr__", SbObject_DefaultGetAttr },
    { "__setattr__", SbObject_DefaultSetAttr },
    { "__delattr__", SbObject_DefaultDelAttr },
    /* Sentinel */
    { NULL, NULL },
};

int
_Sb_TypeInit_Module()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("module", NULL, module_methods, sizeof(SbModuleObject));
    if (!tp) {
        return -1;
    }

    tp->tp_flags = SbType_FLAGS_HAS_DICT;
    tp->tp_dictoffset = Sb_OffsetOf(SbModuleObject, dict);
    tp->tp_destroy = (SbDestroyFunc)module_destroy;

    SbModule_Type = tp;
    return 0;
}

