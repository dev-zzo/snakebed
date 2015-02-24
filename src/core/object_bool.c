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

/* Builtins initializer */
int
_SbBool_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("bool", SbInt_Type);
    if (!tp) {
        return -1;
    }

    SbBool_Type = tp;
    return 0;
}

int
_SbBool_BuiltinInit2()
{
    return 0;
}
