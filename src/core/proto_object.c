#include "snakebed.h"
#include "object.h"
#include "object_type.h"

long
SbObject_Hash(SbObject *p)
{
    SbTypeObject *type;

    type = Sb_TYPE(p);
    if (type->tp_hash) {
        return type->tp_hash(p);
    }

    return -1;
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
