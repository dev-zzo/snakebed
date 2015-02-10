#ifndef __SNAKEBED_OBJECT_H
#define __SNAKEBED_OBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h"

struct _SbTypeObject;
typedef struct _SbTypeObject SbTypeObject;

#define SbObject_HEAD \
    Sb_ssize_t ob_refcount; \
    SbTypeObject *ob_type

typedef struct _SbObject {
    SbObject_HEAD;
} SbObject;

#define Sb_REFCNT(ob) \
    (((SbObject *)(ob))->ob_refcount)
#define Sb_TYPE(ob) \
    (((SbObject *)(ob))->ob_type) 

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_H
