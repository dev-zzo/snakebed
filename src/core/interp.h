#ifndef __SNAKEBED_INTERP_H
#define __SNAKEBED_INTERP_H
#ifdef __cplusplus
extern "C" {
#endif

int 
SbInterp_PushFrame(SbFrameObject *f, SbObject *args, SbObject *kwargs);

int
SbInterp_ExecuteNext(void);


#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_INTERP_H
