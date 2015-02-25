#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbModule_Type = NULL;

SbObject *
SbModule_New(const char *name)
{
    SbModuleObject *op;
    SbObject *name_str;
    SbObject *dict;

    op = (SbModuleObject *)SbObject_New(SbModule_Type);
    if (!op) {
        goto fail0;
    }

    name_str = SbStr_FromString(name);
    if (!name_str) {
        goto fail1;
    }

    dict = SbDict_New();
    if (SbDict_SetItemString(dict, "__name__", name_str) < 0) {
        goto fail2;
    }

    op->dict = dict;

    Sb_DECREF(name_str);
    return (SbObject *)op;

fail2:
    Sb_DECREF(name_str);
fail1:
    Sb_DECREF(op);
fail0:
    return NULL;
}

static void
module_destroy(SbModuleObject *self)
{
    Sb_CLEAR(self->dict);
    SbObject_DefaultDestroy((SbObject *)self);
}

/* Type initializer */

static const SbCMethodDef module_methods[] = {
    { "__setattr__", SbObject_DefaultSetAttr },
    { "__delattr__", SbObject_DefaultDelAttr },

    /* Sentinel */
    { NULL, NULL },
};

int
_SbModule_TypeInit()
{
    SbTypeObject *tp;

    tp = SbType_New("module", NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbModuleObject);
    tp->tp_flags = SbType_FLAGS_HAS_DICT;
    tp->tp_dictoffset = Sb_OffsetOf(SbModuleObject, dict);
    tp->tp_destroy = (destructor)module_destroy;

    SbModule_Type = tp;
    return SbType_CreateMethods(SbModule_Type, module_methods);
}

