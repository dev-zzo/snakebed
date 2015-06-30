#include "snakebed.h"
#include "internal.h"

/*
 * `None` type/object
 */

/* Define the str object structure. */
typedef struct _SbNoneObject {
    SbObject_HEAD;
} SbNoneObject;

/* Keep the type object here. */
SbTypeObject *SbNone_Type = NULL;
SbObject *Sb_None = NULL;

/* Builtins initializer */
int
_SbNone_BuiltinInit()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("None", NULL, NULL, sizeof(SbNoneObject));
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
    SbObject_HEAD;
} SbNotImplementedObject;

/* Keep the type object here. */
SbTypeObject *SbNotImplemented_Type = NULL;
SbObject *Sb_NotImplemented = NULL;

/* Builtins initializer */
int
_SbNotImplemented_BuiltinInit()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("NotImplemented", NULL, NULL, sizeof(SbNotImplementedObject));
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



