#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbBool_Type = NULL;

SbObject *Sb_False = NULL;
SbObject *Sb_True = NULL;

SbObject *
SbBool_FromLong(long x)
{
    if (x) {
        Sb_RETURN_TRUE;
    }
    else {
        Sb_RETURN_FALSE;
    }
}

/* Type initializer */

static SbObject *
bool_new(long val)
{
    SbBoolObject *op;
    op = (SbBoolObject *)SbObject_New(SbBool_Type);
    if (op) {
        op->value = val;
    }
    return (SbObject *)op;
}

int
_Sb_TypeInit_Bool()
{
    SbTypeObject *tp;

    tp = SbType_New("bool", SbInt_Type);
    if (!tp) {
        return -1;
    }

    SbBool_Type = tp;

    Sb_False = bool_new(0);
    Sb_True = bool_new(1);

    return 0;
}
