#include "snakebed.h"
#include "internal.h"

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

/* Type initializer */

static const SbCMethodDef bool_methods[] = {
    /* Sentinel */
    { NULL, NULL },
};

int
_Sb_TypeInit_Bool()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("bool", SbInt_Type, bool_methods, sizeof(SbBoolObject));
    if (!tp) {
        return -1;
    }

    SbBool_Type = tp;

    Sb_False = bool_new(0);
    Sb_True = bool_new(1);
    return 0;
}
