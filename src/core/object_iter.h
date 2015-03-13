#ifndef __SNAKEBED_OBJECT_ITER_H
#define __SNAKEBED_OBJECT_ITER_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SbIterObject {
    SbObject_HEAD;
    SbObject *iterable;
    SbObject *sentinel;
    Sb_ssize_t index;
} SbIterObject;

extern SbTypeObject *SbIter_Type;


#ifdef __cplusplus
}
#endif
#endif /* __SNAKEBED_OBJECT_ITER_H */
