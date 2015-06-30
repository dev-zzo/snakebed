#ifndef __SNAKEBED_OBJECT_BOOL_H
#define __SNAKEBED_OBJECT_BOOL_H
#ifdef __cplusplus
extern "C" {
#endif

/* Define the C function object structure. */
typedef SbIntObject SbBoolObject;

extern SbTypeObject *SbBool_Type;

extern SbObject *Sb_False;
extern SbObject *Sb_True;

#define SbBool_Check(p) \
    (Sb_TYPE(p) == SbBool_Type)

SbObject *
SbBool_FromLong(long x);

#define Sb_RETURN_TRUE \
    do { Sb_INCREF(Sb_True); return Sb_True; } while(0)

#define Sb_RETURN_FALSE \
    do { Sb_INCREF(Sb_False); return Sb_False; } while(0)

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_BOOL_H
