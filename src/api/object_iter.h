#ifndef __SNAKEBED_OBJECT_ITER_H
#define __SNAKEBED_OBJECT_ITER_H
#ifdef __cplusplus
extern "C" {
#endif

struct _SbIterObject;
typedef struct _SbIterObject SbIterObject;

typedef SbObject *(SbIter_NextProc)(SbIterObject *);
typedef void (SbIter_CleanupProc)(SbIterObject *);

struct _SbIterObject {
    SbObject_HEAD;
    SbIter_CleanupProc *cleanupproc;
    SbIter_NextProc *nextproc;
    union {
        struct {
            SbObject *iterable;
            SbObject *sentinel;
        } with_sentinel;
        struct {
            SbObject *iterable;
            SbInt_Native_t index;
        } with_iterable;
        struct {
            SbObject **cursor;
            SbObject **end;
        } with_array;
    } u;
    SbInt_Native_t index;
};

extern SbTypeObject *SbIter_Type;

/* Construct an iterator over a sequence `o`.
   Returns: New reference. */
SbObject *
SbIter_New(SbObject *o);
/* Construct an iterator over a callable `o` with `sentinel` as the end-of-iteration marker.
   Returns: New reference. */
SbObject *
SbIter_New2(SbObject *o, SbObject *sentinel);
/* Construct an iterator over an array starting at `base`.
   NOTE: For internal use only.
   Returns: New reference. */
SbObject *
SbArrayIter_New(SbObject **base, SbObject **end);

/* Return the next value from the iteration `o`.
   Returns: New reference or NULL on no more items or failure. */
SbObject *
SbIter_Next(SbObject *o);

#ifdef __cplusplus
}
#endif
#endif /* __SNAKEBED_OBJECT_ITER_H */
