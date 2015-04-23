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
SbSlice_GetIndices(SbObject *self, SbInt_Native_t seq_length, 
    SbInt_Native_t *start, SbInt_Native_t *end, SbInt_Native_t *step, SbInt_Native_t *slice_length);

#ifdef __cplusplus
}
#endif
#endif /* __SNAKEBED_OBJECT_SLICE_H */
