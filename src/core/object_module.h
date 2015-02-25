#ifndef __SNAKEBED_OBJECT_BOOL_H
#define __SNAKEBED_OBJECT_BOOL_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SbModuleObject {
    SbObject_HEAD;
    SbObject *dict;
} SbModuleObject;

extern SbTypeObject *SbModule_Type;

SbObject *
SbModule_New(const char *name);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_BOOL_H
