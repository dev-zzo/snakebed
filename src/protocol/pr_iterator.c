#include "snakebed.h"

SbObject *
SbObject_GetIter(SbObject *o)
{
    return SbObject_CallMethod(o, "__iter__", NULL, NULL);
}

SbObject *
SbIter_Next(SbObject *o)
{
    SbObject *r;

    r = SbObject_CallMethod(o, "next", NULL, NULL);
    if (!r && SbExc_ExceptionTypeMatches(SbErr_Occurred(), (SbObject *)SbExc_StopIteration)) {
        SbErr_Clear();
    }
    return r;
}
