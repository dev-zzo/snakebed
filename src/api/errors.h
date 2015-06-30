#ifndef __SNAKEBED_EXCEPTION_H
#define __SNAKEBED_EXCEPTION_H
#ifdef __cplusplus
extern "C" {
#endif

/* Get the exception object, if any occurred.
   Returns: Borrowed reference. */
SbObject *
SbErr_Occurred(void);

/* Check whether the `exc` matches anything in `what`.
   Typically, `exc` is what is returned by SbErr_Occurred().
   Returns: 1 if yes, 0 if no, -1 on failure. */
int
SbExc_ExceptionMatches(SbObject *exc, SbObject *what);
int
SbExc_ExceptionTypeMatches(SbTypeObject *exc_type, SbObject *what);

/* Clear error indicator. */
void
SbErr_Clear(void);

/* Retrieve exception information, clearing the current state.
   Note: you own the references. */
void
SbErr_Fetch(SbObject **exc);

/* Reset the exception information back from info.
   Note: references are stolen. */
void
SbErr_Restore(SbObject *exc);


/* Raise an exception of the given type with the associated value.
   Note: the reference to `value` is stolen.
*/
void
SbErr_RaiseWithObject(SbTypeObject *type, SbObject *value);

/* Raise an exception of the given type with the associated value.
   This lets you specify a C string. */
void
SbErr_RaiseWithString(SbTypeObject *type, const char *value);

void
SbErr_RaiseWithFormat(SbTypeObject *type, const char *format, ...);

/* Raise an IOError exception, passing the given args. */
void
SbErr_RaiseIOError(SbInt_Native_t error_code, const char *strerror);

/* Special cases where only one instance of exception exists ever.
   Returns: always NULL. */

SbObject *
SbErr_NoMemory(void);
SbObject *
SbErr_NoMoreItems(void);
    
#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_EXCEPTION_H
