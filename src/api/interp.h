#ifndef __SNAKEBED_INTERP_H
#define __SNAKEBED_INTERP_H
#ifdef __cplusplus
extern "C" {
#endif

extern SbFrameObject *SbInterp_TopFrame;

/* Execute instructions in the frame.
   Returns: New refernce to the return value. */
SbObject *
SbInterp_Execute(SbFrameObject *frame);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_INTERP_H
