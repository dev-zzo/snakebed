#ifndef __SNAKEBED_EXCEPTION_H
#define __SNAKEBED_EXCEPTION_H
#ifdef __cplusplus
extern "C" {
#endif

/* Built-in exception types. */
extern SbTypeObject *SbErr_Exception;
extern SbTypeObject  *SbErr_StandardError;
extern SbTypeObject   *SbErr_AttributeError;
extern SbTypeObject   *SbErr_LookupError;
extern SbTypeObject    *SbErr_IndexError;
extern SbTypeObject    *SbErr_KeyError;
extern SbTypeObject   *SbErr_MemoryError;
extern SbTypeObject   *SbErr_NameError;
extern SbTypeObject    *SbErr_UnboundLocalError;
extern SbTypeObject   *SbErr_SystemError;
extern SbTypeObject   *SbErr_TypeError;
extern SbTypeObject   *SbErr_ValueError;

/* NOTE: this is not tracked with refcounts. */
typedef struct {
    SbTypeObject *type;
    SbObject *value;
    SbObject *traceback;
} SbExceptionInfo;

/* Get the exception *type* if any occurred.
   Returns: Borrowed reference. */
SbTypeObject *
SbErr_Occurred(void);

/* Check whether the `exc` matches anything in `what`.
   Typically, `exc` is what is returned by SbErr_Occurred().
   Returns: 1 if yes, 0 if no, -1 on failure. */
int
SbErr_ExceptionMatches(SbTypeObject *exc, SbObject *what);

/* Clear error indicator. */
void
SbErr_Clear(void);

/* Retrieve exception information, clearing the current state.
   Note: you own the references. */
void
SbErr_Fetch(SbExceptionInfo *info);

/* Retrieve exception information, NOT clearing the current state.
   Note: you get new references. */
void
SbErr_FetchCopy(SbExceptionInfo *info);

/* Reset the exception information back from info.
   Note: references are stolen. */
void
SbErr_Restore(SbExceptionInfo *info);


/* Raise an exception of the given type with the associated value. */
void
SbErr_RaiseWithObject(SbTypeObject *type, SbObject *value);

/* Raise an exception of the given type with the associated value.
   This lets you specify a C string. */
void
SbErr_RaiseWithString(SbTypeObject *type, const char *value);

void
SbErr_RaiseWithFormat(SbTypeObject *type, const char *format, ...);


SbTypeObject *
SbErr_NewException(const char *name, SbTypeObject *base);

/* Raises a MemoryError.
   Returns: always NULL. */
SbObject *
SbErr_NoMemory(void);
    
#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_EXCEPTION_H
