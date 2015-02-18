#include "snakebed.h"
#include "object.h"
#include "object_type.h"

/*
 * `None` type/object
 */

/* Define the str object structure. */
typedef struct _SbNoneObject {
    SbObject_HEAD_VAR;
} SbNoneObject;

/* Keep the type object here. */
SbTypeObject *SbNone_Type = NULL;
SbObject *Sb_None = NULL;

/* Builtins initializer */
int
_SbNone_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("None", SbObject_Type);
    if (!tp) {
        return -1;
    }

    SbNone_Type = tp;

    Sb_None = SbObject_New(SbNone_Type);
    if (!Sb_None) {
        return -1;
    }

    return 0;
}

/*
 * `NotImplemented` type/object
 */

/* Define the str object structure. */
typedef struct _SbNotImplementedObject {
    SbObject_HEAD_VAR;
} SbNotImplementedObject;

/* Keep the type object here. */
SbTypeObject *SbNotImplemented_Type = NULL;
SbObject *Sb_NotImplemented = NULL;

/* Builtins initializer */
int
_SbNotImplemented_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("NotImplemented", SbObject_Type);
    if (!tp) {
        return -1;
    }

    SbNotImplemented_Type = tp;

    Sb_NotImplemented = SbObject_New(SbNotImplemented_Type);
    if (!Sb_NotImplemented) {
        return -1;
    }

    return 0;
}



