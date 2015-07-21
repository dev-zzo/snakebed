#include "snakebed.h"
#include "internal.h"

#define __TRACE_ALLOCS 0

#if __TRACE_ALLOCS
#include <stdio.h>
#endif /* __TRACE_ALLOCS */

/* Keep the type object here. */
SbTypeObject *SbObject_Type = NULL;

#if SUPPORTS(ALLOC_STATISTICS)
unsigned long SbObject_AliveCount = 0;
#endif

void _SbObject_DecRef(SbObject *op)
{
    Sb_ssize_t new_refcount;
    SbTypeObject *tp;
    SbObject *stored_exc;

    new_refcount = op->ob_refcount - 1;
    if (new_refcount > 0) {
        op->ob_refcount = new_refcount;
        return;
    }
    if (new_refcount < 0) {
        __asm int 3;
    }

    /* NOTE: To avoid mayhem, we store the current exception before actually destroying the object. */
    SbErr_Fetch(&stored_exc);
    tp = Sb_TYPE(op);
    /* NOTE: Here, we might check for __del__ when initialising.
       Work around by checking the type's dict directly. */
    if (SbDict_GetItemString(SbObject_DICT(tp), "__del__")) {
        if (!SbObject_CallMethod(op, "__del__", NULL, NULL)) {
            SbErr_Clear();
            /* TODO: print warning maybe? */
        }
        if (op->ob_refcount > 0) {
            SbErr_Restore(stored_exc);
            return;
        }
    }
    op->ob_refcount = 0;
    tp->tp_destroy(op);
    op = NULL;
    SbErr_Restore(stored_exc);
    Sb_DECREF(tp);

#if SUPPORTS(ALLOC_STATISTICS)
    --SbObject_AliveCount;
#endif
}

SbObject *
SbObject_New(SbTypeObject *type)
{
    SbObject *p;
    p = (SbObject *)type->tp_alloc(type, 0);
    SbObject_INIT(p, type);
    if (type->tp_flags & SbType_FLAGS_HAS_DICT) {
        SbObject_DICT(p) = SbDict_New();
    }
#if __TRACE_ALLOCS
    printf("Object at %p (type %s) allocated.\n", p, type->tp_name);
#endif /* __TRACE_ALLOCS */
#if SUPPORTS(ALLOC_STATISTICS)
    ++SbObject_AliveCount;
#endif
    return p;
}

SbVarObject *
SbObject_NewVar(SbTypeObject *type, Sb_ssize_t count)
{
    SbVarObject *p;
    p = (SbVarObject *)type->tp_alloc(type, count);
    SbObject_INIT_VAR(p, type, count);
    if (type->tp_flags & SbType_FLAGS_HAS_DICT) {
        SbObject_DICT(p) = SbDict_New();
    }
#if __TRACE_ALLOCS
    printf("Object at %p (type %s) allocated.\n", p, type->tp_name);
#endif /* __TRACE_ALLOCS */
#if SUPPORTS(ALLOC_STATISTICS)
    ++SbObject_AliveCount;
#endif
    return p;
}

void
SbObject_DefaultDestroy(SbObject *p)
{
    SbTypeObject *type;

    type = Sb_TYPE(p);
    if (type->tp_flags & SbType_FLAGS_HAS_DICT) {
        Sb_CLEAR(SbObject_DICT(p));
    }
#if __TRACE_ALLOCS
    printf("Object at %p (type %s) being freed.\n", p, type->tp_name);
#endif /* __TRACE_ALLOCS */
    type->tp_free(p);
}

SbObject *
SbObject_DefaultHash(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbInt_FromNative((long)self);
}

SbObject *
SbObject_DefaultGetAttr(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_name;
    SbObject *result;
    const char *attr_name;

    if (SbArgs_Parse("S:name", args, kwargs, &o_name) < 0) {
        return NULL;
    }

    attr_name = SbStr_AsStringUnsafe(o_name);
    /* Some hardcoded values... */
    if (!SbRT_StrCmp(attr_name, "__class__")) {
        result = (SbObject *)Sb_TYPE(self);
        goto return_result;
    }
    if (!SbRT_StrCmp(attr_name, "__dict__")) {
        if (Sb_TYPE(self)->tp_flags & SbType_FLAGS_HAS_DICT) {
            result = (SbObject *)SbObject_DICT(self);
            goto return_result;
        }
    }

    /* If the object has a dict, check it. */
    if (Sb_TYPE(self)->tp_flags & SbType_FLAGS_HAS_DICT) {
        result = SbDict_GetItemString(SbObject_DICT(self), attr_name);
        if (result) {
            goto return_result;
        }
    }

    SbErr_RaiseWithObject(SbExc_AttributeError, o_name);
    return NULL;

return_result:
    Sb_INCREF(result);
    return result;
}

SbObject *
SbObject_DefaultSetAttr(SbObject *self, SbObject *args, SbObject *kwargs)
{
    const char *attr_name;
    SbObject *value;

    if (SbArgs_Parse("s:name,O:value", args, kwargs, &attr_name, &value) < 0) {
        return NULL;
    }

    /* If the object has a dict, modify it. */
    if (Sb_TYPE(self)->tp_flags & SbType_FLAGS_HAS_DICT) {
        if (SbDict_SetItemString(SbObject_DICT(self), attr_name, value) < 0) {
            return NULL;
        }
        Sb_RETURN_NONE;
    }
    return NULL;
}

SbObject *
SbObject_DefaultDelAttr(SbObject *self, SbObject *args, SbObject *kwargs)
{
    const char *attr_name;

    if (SbArgs_Parse("s:name", args, kwargs, &attr_name) < 0) {
        return NULL;
    }

    /* If the object has a dict, modify it. */
    if (Sb_TYPE(self)->tp_flags & SbType_FLAGS_HAS_DICT) {
        if (SbDict_DelItemString(SbObject_DICT(self), attr_name) < 0) {
            return NULL;
        }
        Sb_RETURN_NONE;
    }
    return NULL;
}

SbObject *
SbObject_DefaultStr(SbObject *self, SbObject *args, SbObject *kwargs)
{
    /* Provide the "<`type` instance at %p>" boilerplate representation */
    return SbStr_FromFormat("<%s instance at %p>", Sb_TYPE(self)->tp_name, self);
}

#if SUPPORTS(STR_FORMAT)

static SbObject *
object_format(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *str;
    SbObject *formatted;
    SbObject *spec;

    if (SbArgs_Parse("O:spec", args, kwargs, &spec) < 0) {
        return NULL;
    }

    str = SbObject_Str(self);
    if (!str) {
        return NULL;
    }

    formatted = SbBuiltin_Format(str, spec);
    Sb_DECREF(str);

    return formatted;
}

#endif /* SUPPORTS(STR_FORMAT) */

/* Type initializer */

static const SbCMethodDef object_methods[] = {
    { "__hash__", SbObject_DefaultHash },
    { "__str__", SbObject_DefaultStr },
    { "__repr__", SbObject_DefaultStr },

    { "__getattr__", SbObject_DefaultGetAttr },
    { "__setattr__", SbObject_DefaultSetAttr },
    { "__delattr__", SbObject_DefaultDelAttr },
#if SUPPORTS(STR_FORMAT)
    { "__format__", object_format },
#endif
    /* Sentinel */
    { NULL, NULL },
};

int
_SbObject_TypeInit()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("object", NULL, object_methods, sizeof(SbObject));
    if (!tp) {
        return -1;
    }
    tp->tp_flags = SbType_FLAGS_HAS_DICT;
    tp->tp_dictoffset = Sb_OffsetOf(SbObject, dict);
    SbObject_Type = tp;
    return 0;
}
