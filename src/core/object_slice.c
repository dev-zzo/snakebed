#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbSlice_Type = NULL;

SbObject *
SbSlice_New(SbObject *start, SbObject *end, SbObject *step)
{
    SbObject *self;

    self = SbObject_New(SbSlice_Type);
    if (self) {
        SbSliceObject *myself = (SbSliceObject *)self;

        if (start) {
            Sb_INCREF(start);
            myself->start = start;
        }
        if (end) {
            Sb_INCREF(end);
            myself->end = end;
        }
        if (step) {
            Sb_INCREF(step);
            myself->step = step;
        }
    }

    return self;
}

static void
slice_destroy(SbSliceObject *myself)
{
    Sb_XDECREF(myself->start);
    Sb_XDECREF(myself->end);
    Sb_XDECREF(myself->step);
    SbObject_DefaultDestroy((SbObject *)myself);
}

int
SbSlice_GetIndices(SbObject *self, Sb_ssize_t seq_length, 
    Sb_ssize_t *start, Sb_ssize_t *end, Sb_ssize_t *step, Sb_ssize_t *slice_length)
{
    return -1;
}


int
_Sb_TypeInit_Slice()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("slice", NULL, NULL, sizeof(SbSliceObject));
    if (!tp) {
        return -1;
    }

    tp->tp_destroy = (SbDestroyFunc)slice_destroy;

    SbSlice_Type = tp;
    return 0;
}
