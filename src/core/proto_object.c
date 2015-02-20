#include "snakebed.h"
#include "object.h"
#include "object_type.h"
#include "object_cfunc.h"

long
SbObject_Hash(SbObject *p)
{
    SbObject *result;

    result = SbObject_CallMethod(p, "__hash__", NULL, NULL);

    return result ? SbInt_AsLong(result) : -1;
}

int
SbObject_CompareBool(SbObject *p1, SbObject *p2, int op)
{
    SbTypeObject *type;

    /* No need to look up things for these */
    if (p1 == p2) {
        if (op == Sb_EQ)
            return 1;
        if (op == Sb_NE)
            return 0;
    }

    return -1;
}

SbObject *
SbObject_Call(SbObject *callable, SbObject *args, SbObject *kwargs)
{
    /* Avoid recursion. */
    if (SbCFunction_Check(callable)) {
        return SbCFunction_Call(callable, NULL, args, kwargs);
    }
    if (SbType_Check(callable)) {
        /* TODO: call `__new__`. */
        return NULL;
    }
    /* TODO: Look up `__call__` property. */
    return NULL;
}

SbObject *
SbObject_CallMethod(SbObject *o, const char *method, SbObject *args, SbObject *kwargs)
{
    SbObject *p;

    p = _SbType_FindMethod(o, method);
    if (!p) {
        return NULL;
    }

    return SbObject_Call(p, args, kwargs);
}
