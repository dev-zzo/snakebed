#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbStr_Type = NULL;

/*
 * C interface implementations
 */

int
SbStr_CheckExact(SbObject *p)
{
    return Sb_TYPE(p) == SbStr_Type;
}

SbObject *
SbStr_FromString(const char *v)
{
    Sb_ssize_t len;

    len = Sb_StrLen(v);
    return SbStr_FromStringAndSize(v, len);
}

SbObject *
SbStr_FromStringAndSize(const void *v, Sb_ssize_t len)
{
    SbStrObject *op;

    op = (SbStrObject *)SbObject_NewVar(SbStr_Type, len + 1);
    if (op) {
        /* Ehhh... trickery! */
        Sb_COUNT(op)--;
        op->stored_hash = -1;
        Sb_MemCpy(op->items, v, len);
        op->items[len] = '\0';
    }
    return (SbObject *)op;
}

Sb_ssize_t
SbStr_GetSize(SbObject *p)
{
    if (!SbStr_CheckExact(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-string object passed to a string method");
        return -1;
    }
    return SbStr_GetSizeUnsafe(p);
}

const Sb_byte_t *
SbStr_AsString(SbObject *p)
{
    if (!SbStr_CheckExact(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-string object passed to a string method");
        return NULL;
    }
    return SbStr_AsStringUnsafe(p);
}

long
_SbStr_Hash(SbObject *p)
{
    SbStrObject *op = (SbStrObject *)p;
    long x;

    if (op->stored_hash != -1)
        return op->stored_hash;

    x = _SbStr_HashString(SbStr_AsStringUnsafe(p), SbStr_GetSizeUnsafe(p));
    op->stored_hash = x;
    return x;
}

long
_SbStr_HashString(const Sb_byte_t *p, Sb_ssize_t len)
{
    Sb_ssize_t count = len;
    long x;

    x = *p << 7;
    while (count-- > 0) {
        x = (1000003 * x) ^ *p;
        p++;
    }
    x ^= len;
    if (x == -1) {
        x = -2;
    }
    return x;
}

int
_SbStr_Eq(SbObject *p1, SbObject *p2)
{
    Sb_ssize_t length;

    if (!SbStr_CheckExact(p1) || !SbStr_CheckExact(p2)) {
        return -1;
    }
    length = SbStr_GetSizeUnsafe(p1);
    if (length != SbStr_GetSizeUnsafe(p2)) {
        return 0;
    }
    return Sb_MemCmp(SbStr_AsString(p1), SbStr_AsString(p2), length) == 0;
}

int
_SbStr_EqString(SbObject *p1, const char *p2)
{
    Sb_ssize_t length;

    if (!SbStr_CheckExact(p1)) {
        return -1;
    }
    length = Sb_StrLen(p2);
    if (length != SbStr_GetSizeUnsafe(p1)) {
        return 0;
    }
    return Sb_MemCmp(SbStr_AsString(p1), p2, length) == 0;
}


/* Python accessible methods */

static SbObject *
str_hash(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbInt_FromLong(_SbStr_Hash(self));
}

static SbObject *
str_str(SbObject *self, SbObject *args, SbObject *kwargs)
{
    Sb_INCREF(self);
    return self;
}

static SbObject *
str_len(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbInt_FromLong(SbStr_GetSizeUnsafe(self));
}

static SbObject *
str_eq(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *other;

    other = SbTuple_GetItem(args, 0);
    if (other) {
        return SbBool_FromLong(_SbStr_Eq(self, other));
    }
    return NULL;
}

static SbObject *
str_ne(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *other;

    other = SbTuple_GetItem(args, 0);
    if (other) {
        return SbBool_FromLong(!_SbStr_Eq(self, other));
    }
    return NULL;
}

/* Builtins initializer */
int
_SbStr_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("str", NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbStrObject) - sizeof(char);
    tp->tp_itemsize = sizeof(char);
    tp->tp_destroy = SbObject_Destroy;

    SbStr_Type = tp;
    return 0;
}

static const SbCMethodDef str_methods[] = {
    { "__hash__", str_hash },
    { "__str__", str_str },
    { "__len__", str_len },
    { "__eq__", str_eq },
    { "__ne__", str_ne },

    /* Sentinel */
    { NULL, NULL },
};

int
_SbStr_BuiltinInit2()
{
    return SbType_CreateMethods(SbStr_Type, str_methods);
}
