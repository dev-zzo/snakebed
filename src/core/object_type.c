#include "snakebed.h"

/* Relying on compiler here. */
#include <stdarg.h>

/* Keep the type object here. */
SbTypeObject *SbType_Type = NULL;

SbObject *
SbType_GenericAlloc(SbTypeObject *type, Sb_ssize_t nitems)
{
    SbObject *op;
    Sb_ssize_t size;

    size = type->tp_basicsize + nitems * type->tp_itemsize;
    /* TODO: check against overflows. */

    op = (SbObject *)SbObject_Malloc(size);
    if (!op) {
        return SbErr_NoMemory();
    }

    Sb_BZero(op, size);
    /* Object fields get initialised afterwards */

    return op;
}

static int
type_inherit(SbTypeObject *tp, SbTypeObject *base_type)
{
    tp->tp_base = base_type;
    tp->tp_basicsize = base_type->tp_basicsize;
    tp->tp_itemsize = base_type->tp_itemsize;
    tp->tp_destroy = base_type->tp_destroy;
    return 0;
}

SbTypeObject *
SbType_New(const char *name, SbTypeObject *base_type)
{
    SbTypeObject *tp;

    tp = (SbTypeObject *)SbObject_New(SbType_Type);
    if (!tp) {
        return NULL;
    }

    tp->tp_name = name;
    tp->tp_alloc = SbType_GenericAlloc;
    tp->tp_free = SbObject_Free;
    tp->tp_dictoffset = Sb_OffsetOf(SbTypeObject, tp_dict);

    /* If we have a base -- inherit what's inheritable. */
    if (base_type) {
        type_inherit(tp, base_type);
    }

    return tp;
}

/* Handle destruction of type instances */
static void
type_destroy(SbTypeObject *tp)
{
    if (tp->tp_dict) {
        Sb_DECREF(tp->tp_dict);
    }
    SbObject_DefaultDestroy((SbObject *)tp);
}

int
SbType_CreateMethods(SbTypeObject *type, const SbCMethodDef *methods)
{
    SbObject *dict;
    SbObject *func;

    if (type->tp_dict) {
        dict = type->tp_dict;
    }
    else {
        dict = SbDict_New();
        if (!dict) {
            return -1;
        }
        type->tp_dict = dict;
    }

    while (methods->name) {
        func = SbCFunction_New(methods->func);
        if (!func) {
            return 1;
        }
        if (SbDict_SetItemString(dict, methods->name, func) < 0) {
            return -1;
        }
        Sb_DECREF(func);
        methods++;
    }

    type->tp_flags |= SbType_FLAGS_HAS_DICT;
    return 0;
}

/* Builtins initializer */
int
_SbType_BuiltinInit()
{
    SbTypeObject *tp;
    Sb_size_t size = sizeof(SbTypeObject);

    tp = (SbTypeObject *)SbObject_Malloc(size);
    if (!tp) {
        /* OOM. */
        return -1;
    }

    Sb_BZero(tp, size);
    SbObject_INIT(tp, tp);

    tp->tp_name = "type";
    tp->tp_basicsize = size;
    tp->tp_destroy = (SbDestroyFunc)type_destroy;
    tp->tp_alloc = SbType_GenericAlloc;
    tp->tp_free = SbObject_Free;

    SbType_Type = tp;

    return 0;
}

int
_SbType_BuiltinInit2()
{
    return 0;
}
