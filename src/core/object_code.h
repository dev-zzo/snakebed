#ifndef __SNAKEBED_OBJECT_CODE_H
#define __SNAKEBED_OBJECT_CODE_H
#ifdef __cplusplus
extern "C" {
#endif

/* This heavily depends on what Python 2.7 has. */
typedef struct _SbCodeObject {
    SbObject_HEAD;
    SbObject *name;

    unsigned flags;
    unsigned stack_size; /* Evaluation stack max depth */
    unsigned arg_count; /* Max: 255 */

    SbObject *code; /* str: bytecode itself */
    SbObject *consts; /* constants used */
    SbObject *names; /* names used (strs, pot. interned) */
    SbObject *fastnames; /* these used with {Load|Store|Delete}Fast (strs, pot. interned) */
    /* TBD: closures */
} SbCodeObject;

#define SbCode_NEWLOCALS    (1 << 1)
#define SbCode_VARARGS      (1 << 2)
#define SbCode_VARKWDS      (1 << 3)
#define SbCode_GENERATOR    (1 << 5)
#define SbCode_NO_FREE_VARS (1 << 6)

extern SbTypeObject *SbCode_Type;

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_CODE_H
