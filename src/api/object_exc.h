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
extern SbTypeObject *SbExc_BaseException;
extern SbTypeObject  *SbExc_Exception;
extern SbTypeObject   *SbExc_StandardError;
extern SbTypeObject    *SbExc_AttributeError;
extern SbTypeObject    *SbExc_EnvironmentError;
extern SbTypeObject     *SbExc_IOError;
extern SbTypeObject    *SbExc_ImportError;
extern SbTypeObject    *SbExc_LookupError;
extern SbTypeObject     *SbExc_IndexError;
extern SbTypeObject     *SbExc_KeyError;
extern SbTypeObject    *SbExc_MemoryError;
extern SbTypeObject    *SbExc_NameError;
extern SbTypeObject     *SbExc_UnboundLocalError;
extern SbTypeObject    *SbExc_SystemError;
extern SbTypeObject    *SbExc_TypeError;
extern SbTypeObject    *SbExc_ValueError;
extern SbTypeObject   *SbExc_StopIteration;
extern SbTypeObject  *SbExc_SystemExit;

/* Verify the given object is an exception instance.
   Returns: 1 if true, 0 otherwise. */
int
SbExc_Check(SbObject *o);

/* Create a new exception type.
   Returns: New reference. */
SbTypeObject *
SbExc_NewException(const char *name, SbTypeObject *base);

#define SbExc_GetValue(p) \
    (((SbBaseExceptionObject *)p)->args)

#ifdef __cplusplus
}
#endif
#endif /* __SNAKEBED_OBJECT_EXCEPTION_H */
