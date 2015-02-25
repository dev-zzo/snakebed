#ifndef __SNAKEBED_OBJECT_FRAME_H
#define __SNAKEBED_OBJECT_FRAME_H
#ifdef __cplusplus
extern "C" {
#endif

struct _SbFrameObject;
typedef struct _SbFrameObject SbFrameObject;

typedef struct _SbFrameObject {
    SbObject_HEAD_VAR;
    SbFrameObject *prev; /* Previous frame in frame stack */
    SbCodeObject *code;
    SbObject *globals; /* dict -- global namespace associated with current frame */
    SbObject *locals; /* dict -- local namespace associated with current frame */
    const Sb_byte_t *ip;
    SbObject **sp; /* topmost in stack */
    SbObject *vars[1]; /* locals + stack */
} SbFrameObject;

extern SbTypeObject *SbFrame_Type;

SbObject *
SbFrame_New(SbCodeObject *code, SbFrameObject *prev);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_FRAME_H
