#include "snakebed.h"
#include "object_str.h"
#include "object_type.h"

/* Define the str object structure. */
typedef struct _SbStrObject {
    SbObject_HEAD_VAR;
    long stored_hash;
    char items[1];
} SbStrObject;

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
SbStr_FromStringAndSize(const char *v, Sb_ssize_t len)
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
SbStr_GetSizeUnsafe(SbObject *p)
{
    return Sb_COUNT(p);
}

char *
SbStr_AsStringUnsafe(SbObject *p)
{
    return ((SbStrObject *)p)->items;
}


Sb_ssize_t
SbStr_GetSize(SbObject *p)
{
    if (!SbStr_CheckExact(p)) {
        /* raise TypeError */
        return -1;
    }
    return SbStr_GetSizeUnsafe(p);
}

char *
SbStr_AsString(SbObject *p)
{
    if (!SbStr_CheckExact(p)) {
        /* raise TypeError */
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
_SbStr_HashString(const unsigned char *p, Sb_ssize_t len)
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
        return 0;
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
        return 0;
    }
    length = Sb_StrLen(p2);
    if (length != SbStr_GetSizeUnsafe(p1)) {
        return 0;
    }
    return Sb_MemCmp(SbStr_AsString(p1), SbStr_AsString(p2), length) == 0;
}


/* Python accessible methods */


/* Builtins initializer */
int
_SbStr_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("str", SbObject_Type);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbStrObject) - sizeof(char);
    tp->tp_itemsize = sizeof(char);
    tp->tp_hash = (hashfunc)_SbStr_Hash;

    SbStr_Type = tp;
    return 0;
}
