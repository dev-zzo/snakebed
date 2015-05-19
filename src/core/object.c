#include "snakebed.h"

#define __TRACE_ALLOCS 0

#if __TRACE_ALLOCS
#include <stdio.h>
#endif /* __TRACE_ALLOCS */

/* Keep the type object here. */
SbTypeObject *SbObject_Type = NULL;

#if SUPPORTS_ALLOC_STATISTICS
unsigned long SbObject_AliveCount = 0;
#endif

void _SbObject_DecRef(SbObject *op)
{
    Sb_ssize_t new_refcount;
    SbTypeObject *tp;
    SbObject *stored_exc;

    new_refcount = --op->ob_refcount;
    if (new_refcount > 0) {
        return;
    }
    if (new_refcount < 0) {
        __asm int 3;
    }

    /* NOTE: To avoid mayhem, we store the current exception before actually destroying the object. */
    SbErr_Fetch(&stored_exc);
    tp = Sb_TYPE(op);
    tp->tp_destroy(op);
    op = NULL;
    SbErr_Restore(stored_exc);
    Sb_DECREF(tp);

#if SUPPORTS_ALLOC_STATISTICS
    --SbObject_AliveCount;
#endif
}

SbObject *
SbObject_New(SbTypeObject *type)
{
    SbObject *p;
    p = (SbObject *)type->tp_alloc(type, 0);
    SbObject_INIT(p, type);
    /*
    if (type->tp_flags & SbType_FLAGS_HAS_DICT) {
        SbObject_DICT(p) = SbDict_New();
    }
    */
#if __TRACE_ALLOCS
    printf("Object at %p (type %s) allocated.\n", p, type->tp_name);
#endif /* __TRACE_ALLOCS */
#if SUPPORTS_ALLOC_STATISTICS
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
    /*
    if (type->tp_flags & SbType_FLAGS_HAS_DICT) {
        SbObject_DICT(p) = SbDict_New();
    }
    */
#if __TRACE_ALLOCS
    printf("Object at %p (type %s) allocated.\n", p, type->tp_name);
#endif /* __TRACE_ALLOCS */
#if SUPPORTS_ALLOC_STATISTICS
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
SbObject_DefaultSetAttr(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *attr_name;
    SbObject *value;

    if (SbArgs_Unpack(args, 2, 2, &attr_name, &value) < 0) {
        return NULL;
    }
    if (!SbStr_CheckExact(attr_name)) {
        SbErr_RaiseWithString(SbErr_TypeError, "attribute name must be a string");
        return NULL;
    }

    /* If the object has a dict, modify it. */
    if (Sb_TYPE(self)->tp_flags & SbType_FLAGS_HAS_DICT) {
        if (SbDict_SetItemString(SbObject_DICT(self), SbStr_AsString(attr_name), value) < 0) {
            return NULL;
        }
        Sb_RETURN_NONE;
    }
    return NULL;
}

SbObject *
SbObject_DefaultDelAttr(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *attr_name;

    if (SbArgs_Unpack(args, 1, 1, &attr_name) < 0) {
        return NULL;
    }
    if (!SbStr_CheckExact(attr_name)) {
        SbErr_RaiseWithString(SbErr_TypeError, "attribute name must be a string");
        return NULL;
    }

    /* If the object has a dict, modify it. */
    if (Sb_TYPE(self)->tp_flags & SbType_FLAGS_HAS_DICT) {
        if (SbDict_DelItemString(SbObject_DICT(self), SbStr_AsString(attr_name)) < 0) {
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

/* Type initializer */

static const SbCMethodDef object_methods[] = {
    { "__hash__", SbObject_DefaultHash },
    { "__str__", SbObject_DefaultStr },
    { "__repr__", SbObject_DefaultStr },

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
    SbObject_Type = tp;
    return 0;
}
