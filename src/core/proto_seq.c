#include "snakebed.h"

Sb_ssize_t
SbSequence_GetSize(SbObject *o)
{
    return SbObject_GetSize(o);
}

SbObject *
SbSequence_GetItem(SbObject *o, Sb_ssize_t index)
{
    return NULL;
}

int
SbSequence_SetItem(SbObject *o, Sb_ssize_t index, SbObject *value)
{
    return -1;
}

int
SbSequence_DelItem(SbObject *o, Sb_ssize_t index)
{
    return -1;
}
