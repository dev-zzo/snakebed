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

/* Generate a traceback object from the currently topmost interpreter frame.
   Returns: New reference. */
SbObject *
SbTraceBack_FromHere();

/* Write out exception and traceback to a file.
   Returns: 0 if OK, -1 otherwise. */
int
SbTraceBack_PrintException(SbTypeObject *type, SbObject *value, SbObject *tb, Sb_ssize_t limit, SbObject *file);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_TRACEBACK_H
