#ifndef __SNAKEBED_OBJECT_SLICE_H
#define __SNAKEBED_OBJECT_SLICE_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SbSliceObject {
    SbObject_HEAD;
    SbObject *start;
    SbObject *end;
    SbObject *step;
} SbSliceObject;

extern SbTypeObject *SbSlice_Type;

#define SbSlice_Check(p) \
    (Sb_TYPE(p) == SbSlice_Type)

SbObject *
SbSlice_New(SbObject *start, SbObject *end, SbObject *step);

int
SbSlice_GetIndices(SbObject *self, Sb_ssize_t seq_length, 
    Sb_ssize_t *start, Sb_ssize_t *end, Sb_ssize_t *step, Sb_ssize_t *slice_length);

#ifdef __cplusplus
}
#endif
#endif /* __SNAKEBED_OBJECT_SLICE_H */
