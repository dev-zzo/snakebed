#include "snakebed.h"

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

SbTypeObject *
SbType_New(const char *name, SbTypeObject *base_type, SbObject *dict)
{
    SbTypeObject *tp;

    tp = (SbTypeObject *)SbObject_New(SbType_Type);
    if (!tp) {
        goto fail0;
    }

    tp->tp_name = name;
    tp->tp_alloc = SbType_GenericAlloc;
    tp->tp_free = SbObject_Free;
    if (!dict && SbDict_Type) {
        dict = SbDict_New();
        if (!dict) {
            goto fail1;
        }
    }
    else if (dict) {
        Sb_INCREF(dict);
    }
    tp->tp_dict = dict;
    if (base_type) {
        Sb_INCREF(base_type);
        tp->tp_base = base_type;
        tp->tp_basicsize = base_type->tp_basicsize;
        tp->tp_itemsize = base_type->tp_itemsize;
        tp->tp_destroy = base_type->tp_destroy;
        if (SbDict_Merge(tp->tp_dict, base_type->tp_dict, 0) < 0) {
            goto fail1;
        }
    }

    return tp;

fail1:
    Sb_DECREF(tp);
fail0:
    return NULL;
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

SbObject *
_SbType_New(SbObject *name, SbObject *bases, SbObject *dict)
{
    Sb_ssize_t base_count;
    SbTypeObject *base;
    SbTypeObject *result;

    if (!SbStr_CheckExact(name)) {
        SbErr_RaiseWithString(SbErr_TypeError, "expected `name` to be a str");
        return NULL;
    }
    if (!SbTuple_CheckExact(bases)) {
        SbErr_RaiseWithString(SbErr_TypeError, "expected `base` to be a tuple");
        return NULL;
    }
    base_count = SbTuple_GetSizeUnsafe(bases);
    if (base_count > 1) {
        SbErr_RaiseWithString(SbErr_TypeError, "expected `base` to contain only one type");
        return NULL;
    }
    base = (SbTypeObject *)(base_count > 0 ? SbTuple_GetItemUnsafe(bases, 0) : NULL);
    if (!SbDict_CheckExact(dict)) {
        SbErr_RaiseWithString(SbErr_TypeError, "expected `dict` to be a dict");
        return NULL;
    }

    /* Hacky: keep a ref to the type name in type dict... */
    if (SbDict_SetItemString(dict, "__name__", name) < 0) {
        return NULL;
    }

    result = SbType_New(SbStr_AsStringUnsafe(name), base, dict);
    if (!result) {
        return NULL;
    }

    result->tp_basicsize = sizeof(SbObject);
    result->tp_flags = SbType_FLAGS_HAS_DICT;
    result->tp_dictoffset = Sb_OffsetOf(SbObject, dict);
    result->tp_destroy = SbObject_DefaultDestroy;

    return (SbObject *)result;
}

static SbObject *
type_new(SbObject *cls, SbObject *args, SbObject *kwargs)
{
    SbObject *name = NULL, *base = NULL, *dict = NULL;

    if (SbArgs_Unpack(args, 4, 4, &cls, &name, &base, &dict) < 0) {
        return NULL;
    }
    return _SbType_New(name, base, dict);
}

static SbObject *
type_init(SbTypeObject *self, SbObject *args, SbObject *kwargs)
{
    /* Empty! Should verify that args/kwds are empty, but... */
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

    tp->tp_flags = SbType_FLAGS_HAS_DICT;
    tp->tp_dictoffset = Sb_OffsetOf(SbTypeObject, tp_dict);
    tp->tp_dict = _SbType_BuildMethodDict(type_methods);

    return 0;
}
