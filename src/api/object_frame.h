#ifndef __SNAKEBED_OBJECT_FRAME_H
#define __SNAKEBED_OBJECT_FRAME_H
#ifdef __cplusplus
extern "C" {
#endif

struct _SbFrameObject;
typedef struct _SbFrameObject SbFrameObject;

/* This is used to keep track of which code blocks we have entered. */
typedef struct _SbCodeBlock {
    struct _SbCodeBlock *next;
    /* Where do we go on `break` */
    const Sb_byte_t *handler;
    /* How much do we pop from stack */
    SbObject **old_sp;
    /* Which instruction caused the block to be pushed */
    Sb_byte_t setup_insn;
} SbCodeBlock;

typedef struct _SbFrameObject {
    SbObject_HEAD_VAR;
    SbFrameObject *prev; /* Previous frame in frame stack */
    SbCodeObject *code;
    SbObject *globals; /* dict -- global namespace associated with current frame */
    SbObject *locals; /* dict -- local namespace associated with current frame */
    SbCodeBlock *blocks;
    SbObject *current_exc;
    const Sb_byte_t *ip;
    SbObject **sp; /* topmost in stack */
    SbObject *stack[1]; /* stack */
} SbFrameObject;

extern SbTypeObject *SbFrame_Type;

SbFrameObject *
SbFrame_New(SbCodeObject *code, SbObject *globals, SbObject *locals);

int
SbFrame_SetPrevious(SbFrameObject *f, SbFrameObject *prev);

int
SbFrame_ApplyArgs(SbFrameObject *f, SbObject *args, SbObject *kwds, SbObject *defaults);

/* Pushes a new block on top of the current block stack */
int
SbFrame_PushBlock(SbFrameObject *f, const Sb_byte_t *handler, SbObject **old_sp, Sb_byte_t setup_insn);

/* Pops and frees the topmost block */
void
SbFrame_PopBlock(SbFrameObject *f);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_FRAME_H
