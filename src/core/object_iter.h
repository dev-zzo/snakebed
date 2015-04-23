#ifndef __SNAKEBED_OBJECT_ITER_H
#define __SNAKEBED_OBJECT_ITER_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SbIterObject {
    SbObject_HEAD;
    SbObject *iterable;
    SbObject *sentinel;
    SbInt_Native_t index;
} SbIterObject;

extern SbTypeObject *SbIter_Type;

/* Return the next value from the iteration `o`.
   Returns: New reference or NULL on failure. */
SbObject *
SbIter_Next(SbObject *o);

#ifdef __cplusplus
}
#endif
#endif /* __SNAKEBED_OBJECT_ITER_H */
