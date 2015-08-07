#ifndef __SNAKEBED_OBJECT_TRACEBACK_H
#define __SNAKEBED_OBJECT_TRACEBACK_H
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"

/* Define the tuple object structure. */
typedef struct _SbTraceBackObject SbTraceBackObject;
struct _SbTraceBackObject {
    SbObject_HEAD;
    SbTraceBackObject *next;
    SbFrameObject *frame;
    unsigned ip;
};

extern SbTypeObject *SbTraceBack_Type;

SbObject *
SbTraceBack_FromHere();

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_TRACEBACK_H
