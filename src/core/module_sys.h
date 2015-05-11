#ifndef __SNAKEBED_MODULE_SYS_H
#define __SNAKEBED_MODULE_SYS_H
#ifdef __cplusplus
extern "C" {
#endif

/* Implements the `sys` module. */

/* The `sys` module itself. */
extern SbObject *Sb_ModuleSys;
/* The `sys.modules` attribute. */
extern SbObject *SbSys_Modules;
/* The `sys.stdin` and friends. */
extern SbObject *SbSys_StdIn;
extern SbObject *SbSys_StdOut;
extern SbObject *SbSys_StdErr;

#ifdef __cplusplus
}
#endif
#endif /* __SNAKEBED_MODULE_SYS_H */
