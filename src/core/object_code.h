#ifndef __SNAKEBED_OBJECT_CODE_H
#define __SNAKEBED_OBJECT_CODE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"

typedef struct _SbCodeObject {
    SbObject_HEAD;
} SbCodeObject;

extern SbTypeObject *SbCode_Type;

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_CODE_H
