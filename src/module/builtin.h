#ifndef __SNAKEBED_MODULE_BUILTIN_H
#define __SNAKEBED_MODULE_BUILTIN_H
#ifdef __cplusplus
extern "C" {
#endif

/* Implements the `__builtin__` module. */
    
extern SbObject *Sb_ModuleBuiltin;

SbObject *
SbBuiltin_Format(SbObject *self, SbObject *spec);

#ifdef __cplusplus
}
#endif
#endif /* __SNAKEBED_MODULE_BUILTIN_H */
