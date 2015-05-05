#ifndef __SNAKEBED_OBJECT_EXCEPTION_H
#define __SNAKEBED_OBJECT_EXCEPTION_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SbBaseExceptionObject {
    SbObject_HEAD;
    SbObject *args;
} SbBaseExceptionObject;

/* Built-in exception types. */
extern SbTypeObject *SbErr_BaseException;
extern SbTypeObject  *SbErr_Exception;
extern SbTypeObject   *SbErr_StandardError;
extern SbTypeObject    *SbErr_AttributeError;
extern SbTypeObject    *SbErr_EnvironmentError;
extern SbTypeObject     *SbErr_IOError;
extern SbTypeObject    *SbErr_LookupError;
extern SbTypeObject     *SbErr_IndexError;
extern SbTypeObject     *SbErr_KeyError;
extern SbTypeObject    *SbErr_MemoryError;
extern SbTypeObject    *SbErr_NameError;
extern SbTypeObject     *SbErr_UnboundLocalError;
extern SbTypeObject    *SbErr_SystemError;
extern SbTypeObject    *SbErr_TypeError;
extern SbTypeObject    *SbErr_ValueError;
extern SbTypeObject   *SbErr_StopIteration;
extern SbTypeObject  *SbErr_SystemExit;

/* Create a new exception type.
   Returns: New reference. */
SbTypeObject *
SbErr_NewException(const char *name, SbTypeObject *base);

/* INTERNAL USE ONLY */

SbObject *
_SbErr_Instantiate(SbTypeObject *type, SbObject *value);

#ifdef __cplusplus
}
#endif
#endif /* __SNAKEBED_OBJECT_EXCEPTION_H */
