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

SbObject *
SbType_GenericNew(SbObject *dummy, SbObject *args, SbObject *kwds)
{
    SbTypeObject *tp;
    SbObject *o;

    tp = (SbTypeObject *)SbTuple_GetItem(args, 0);
    if (tp->tp_itemsize) {
        SbErr_RaiseWithString(SbErr_TypeError, "can't create a varobject with generic `__new__`");
        return NULL;
    }

    o = SbObject_New(tp);
    return o;
}


static int
type_inherit(SbTypeObject *tp, SbTypeObject *base_type)
{
    Sb_INCREF(base_type);
    tp->tp_base = base_type;
    tp->tp_basicsize = base_type->tp_basicsize;
    tp->tp_itemsize = base_type->tp_itemsize;
    tp->tp_destroy = base_type->tp_destroy;
    tp->tp_dict = SbDict_Copy(base_type->tp_dict);
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
    if (SbDict_Type) {
        tp->tp_dict = SbDict_New();
    }

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
    Sb_XDECREF(tp->tp_base);
    Sb_CLEAR(tp->tp_dict);
    SbObject_DefaultDestroy((SbObject *)tp);
}

int
SbType_IsSubtype(SbTypeObject *a, SbTypeObject *b)
{
    while (a) {
        if (a == b) {
            return 1;
        }
        a = a->tp_base;
    }

    return 0;
}

int
SbType_CreateMethods(SbTypeObject *type, const SbCMethodDef *methods)
{
    SbObject *dict;
    SbObject *func;

    dict = type->tp_dict;
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

    return 0;
}

/* Python accessible methods */

static SbObject *
type_new(SbObject *dummy, SbObject *args, SbObject *kwargs)
{
    Sb_ssize_t count;
    SbObject *name = NULL, *base = NULL, *dict = NULL;
    SbObject *result;

    if (SbTuple_Unpack(args, 1, 3, &name, &base, &dict) < 0) {
        return NULL;
    }

    count = SbTuple_GetSizeUnsafe(args);
    if (count == 2) {
        SbErr_RaiseWithString(SbErr_TypeError, "type() takes 1 or 3 parameters");
        return NULL;
    }

    if (!base && !dict) {
        result = (SbObject *)Sb_TYPE(name);
        Sb_INCREF(result);
        return result;
    }

    if (!SbStr_CheckExact(name)) {
        SbErr_RaiseWithString(SbErr_TypeError, "expected `name` to be a str");
        return NULL;
    }
    if (!SbStr_CheckExact(base)) {
        SbErr_RaiseWithString(SbErr_TypeError, "expected `base` to be a type");
        return NULL;
    }
    if (!SbDict_CheckExact(dict)) {
        SbErr_RaiseWithString(SbErr_TypeError, "expected `dict` to be a dict");
        return NULL;
    }

    result = (SbObject *)SbType_New(SbStr_AsStringUnsafe(name), (SbTypeObject *)base);
    return result;
}

static SbObject *
type_init(SbTypeObject *self, SbObject *args, SbObject *kwargs)
{
    /* TODO */
    Sb_RETURN_NONE;
}

static SbObject *
type_call(SbTypeObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o;
    SbObject *m;
    SbObject *new_args;

    new_args = _SbTuple_Prepend((SbObject *)self, args);
    if (!new_args) {
        return NULL;
    }

    m = SbDict_GetItemString(SbObject_DICT(self), "__new__");
    if (m) {
        o = SbObject_Call(m, new_args, kwargs);
    }
    else {
        o = SbType_GenericNew(NULL, new_args, kwargs);
    }
    Sb_DECREF(new_args);
    if (!o) {
        return NULL;
    }

    if (SbType_IsSubtype(Sb_TYPE(o), self)) {
        /* Just don't call `__init__` if it's not there. */
        m = SbDict_GetItemString(SbObject_DICT(self), "__init__");
        if (m) {
            SbObject *none;

            none = SbObject_CallMethod(o, "__init__", args, kwargs);
            Sb_XDECREF(none);
            if (none == NULL) {
                Sb_DECREF(o);
                o = NULL;
            }
        }
    }

    return o;
}

/* Type initializer */

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

static const SbCMethodDef type_methods[] = {
    { "__new__", (SbCFunction)type_new },
    { "__init__", (SbCFunction)type_init },
    { "__call__", (SbCFunction)type_call },
    /* Sentinel */
    { NULL, NULL },
};

int
_SbType_BuiltinInit2()
{
    SbTypeObject *tp = SbType_Type;

    tp->tp_flags = SbType_FLAGS_HAS_DICT;
    tp->tp_dictoffset = Sb_OffsetOf(SbTypeObject, tp_dict);
    tp->tp_dict = SbDict_New();

    return SbType_CreateMethods(tp, type_methods);
}
