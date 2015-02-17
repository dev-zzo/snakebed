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

    tp = SbType_New("None", SbNone_Type);
    if (!tp) {
        return -1;
    }

    SbNone_Type = tp;
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

    tp = SbType_New("NotImplemented", SbNotImplemented_Type);
    if (!tp) {
        return -1;
    }

    SbNotImplemented_Type = tp;
    return 0;
}



