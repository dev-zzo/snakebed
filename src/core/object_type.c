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

/*
static int
type_inherit(SbTypeObject *tp, SbTypeObject *base_type)
{
    tp->tp_basicsize = base_type->tp_basicsize;
    tp->tp_itemsize = base_type->tp_itemsize;
    tp->tp_destroy = base_type->tp_destroy;
    tp->tp_dict = SbDict_Copy(base_type->tp_dict);
    return 0;
}
*/

/* The part that corresponds to `__new__()` */
static SbTypeObject *
type_do_new(const char *name, SbTypeObject *base_type)
{
    SbTypeObject *tp;

    tp = (SbTypeObject *)SbObject_New(SbType_Type);
    if (!tp) {
        return NULL;
    }

    tp->tp_name = name;
    tp->tp_alloc = SbType_GenericAlloc;
    tp->tp_free = SbObject_Free;
    if (base_type) {
        Sb_INCREF(base_type);
        tp->tp_base = base_type;
    }

    return tp;
}

/* The part that corresponds to `__init__()` */
int
_SbType_Init(SbTypeObject *tp, SbObject *dict)
{
    SbTypeObject *base = tp->tp_base;

    if (!dict && SbDict_Type) {
        dict = SbDict_New();
    }
    tp->tp_dict = dict;
    if (base) {
        tp->tp_basicsize = base->tp_basicsize;
        tp->tp_itemsize = base->tp_itemsize;
        tp->tp_destroy = base->tp_destroy;
        SbDict_Merge(tp->tp_dict, base->tp_dict, 0);
    }
    return 0;
}

SbTypeObject *
SbType_New(const char *name, SbTypeObject *base_type, SbObject *dict)
{
    SbTypeObject *tp;

    tp = type_do_new(name, base_type);
    if (!tp) {
        return NULL;
    }
    _SbType_Init(tp, dict);
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

/* Python accessible methods */

static SbObject *
type_new(SbObject *cls, SbObject *args, SbObject *kwargs)
{
    SbObject *name = NULL, *base = NULL, *dict = NULL;
    SbObject *result;

    if (SbTuple_Unpack(args, 4, 4, &cls, &name, &base, &dict) < 0) {
        return NULL;
    }

    if (!SbStr_CheckExact(name)) {
        SbErr_RaiseWithString(SbErr_TypeError, "expected `name` to be a str");
        return NULL;
    }
    if (!SbTuple_CheckExact(base)) {
        SbErr_RaiseWithString(SbErr_TypeError, "expected `base` to be a tuple");
        return NULL;
    }
    if (SbTuple_GetSizeUnsafe(base) > 1) {
        SbErr_RaiseWithString(SbErr_TypeError, "expected `base` to contain only one type");
        return NULL;
    }
    if (!SbDict_CheckExact(dict)) {
        SbErr_RaiseWithString(SbErr_TypeError, "expected `dict` to be a dict");
        return NULL;
    }

    base = SbTuple_GetItemUnsafe(base, 0);
    result = (SbObject *)type_do_new(SbStr_AsStringUnsafe(name), (SbTypeObject *)base);
    return result;
}

static SbObject *
type_init(SbTypeObject *self, SbObject *args, SbObject *kwargs)
{
    /* TODO */
    Sb_RETURN_NONE;
}

static SbObject *
type_instantiate(SbTypeObject *type, SbObject *args, SbObject *kwargs)
{
    SbObject *o;
    SbObject *m;
    SbObject *new_args;

    new_args = _SbTuple_Prepend((SbObject *)type, args);
    if (!new_args) {
        return NULL;
    }

    m = SbDict_GetItemString(SbObject_DICT(type), "__new__");
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

    /* If `__new__()` does not return an instance of cls, 
     * then the new instance's `__init__()` method will not be invoked. */
    if (SbType_IsSubtype(Sb_TYPE(o), type)) {
        /* Just don't call `__init__` if it's not there. */
        m = SbDict_GetItemString(SbObject_DICT(type), "__init__");
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

static SbObject *
type_call(SbTypeObject *self, SbObject *args, SbObject *kwargs)
{
    if (self == SbType_Type) {
        Sb_ssize_t count;

        count = SbTuple_GetSizeUnsafe(args);
        if (count == 1) {
            SbObject *tp;

            tp = (SbObject *)Sb_TYPE(SbTuple_GetItemUnsafe(args, 0));
            Sb_INCREF(tp);
            return tp;
        }
        if (count != 3) {
            SbErr_RaiseWithString(SbErr_TypeError, "type() takes 1 or 3 parameters");
            return NULL;
        }
        /* Fall through */
    }

    /* Invoke `__new__()` and do with object construction.
     * `self` is actually the type we need to construct.
     */
    return type_instantiate(self, args, kwargs);
}

/* Type initializer */

int
_SbType_BuiltinInit()
{
    SbTypeObject *tp;
    Sb_size_t size = sizeof(SbTypeObject);

    tp = (SbTypeObject *)SbObject_Malloc(size);
    if (!tp) {
        /* Can't raise an exception -- there is no exceptions created yet. */
        /* SbErr_NoMemory(); */
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

    /* Inhibit dictionary creation
    tp->tp_flags = SbType_FLAGS_HAS_DICT;
    */
    tp->tp_dictoffset = Sb_OffsetOf(SbTypeObject, tp_dict);
    tp->tp_dict = _SbType_BuildMethodDict(type_methods);

    return 0;
}
