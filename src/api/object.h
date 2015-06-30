#ifndef __SNAKEBED_OBJECT_H
#define __SNAKEBED_OBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/* Forward declare the type object. */
struct _SbObject;
typedef struct _SbObject SbObject;
struct _SbTypeObject;
typedef struct _SbTypeObject SbTypeObject;

/* Objects that do not support the "variable" part.
   A basic object is composed of:
   - Reference counter
   - Pointer to a type object
   - Pointer to instance variables container (either a dict or a tuple)
 */
#define SbObject_HEAD \
    Sb_ssize_t ob_refcount; \
    SbTypeObject *ob_type

struct _SbObject {
    SbObject_HEAD;
    SbObject *dict;
};

/* Objects that do support the "variable" part. */
#define SbObject_HEAD_VAR \
    SbObject_HEAD; \
    Sb_ssize_t ob_itemcount

typedef struct _SbVarObject {
    SbObject_HEAD_VAR;
} SbVarObject;

/* Macros to easily access the object's fields */
#define Sb_OBJECT(op) ((SbObject *)(op))
#define Sb_REFCNT(op) \
    (Sb_OBJECT(op)->ob_refcount)
#define Sb_TYPE(op) \
    (Sb_OBJECT(op)->ob_type)
#define Sb_VAROBJECT(op) ((SbVarObject *)(op))
#define Sb_COUNT(op) \
    (Sb_VAROBJECT(op)->ob_itemcount)

/* INTERNAL: Decrease the refcount and deallocate the instance if required. */
extern void _SbObject_DecRef(SbObject *op);

#define Sb_INCREF(op) \
    (Sb_OBJECT(op)->ob_refcount++)
#define Sb_XINCREF(op) \
    do { if ((op) == NULL) break; Sb_INCREF(op); } while (0)
#define Sb_DECREF(op) \
    _SbObject_DecRef(Sb_OBJECT(op))
#define Sb_XDECREF(op) \
    do { if ((op) == NULL) break; Sb_DECREF(op); } while (0)
#define Sb_CLEAR(op) \
    do { \
        if (op) { \
            SbObject *_py_tmp = Sb_OBJECT(op); \
            (op) = NULL; \
            Sb_DECREF(_py_tmp); \
        } \
    } while (0)

/* Initialize the object's fields */
#define SbObject_INIT(op, type) \
    do { Sb_REFCNT(op) = 1; Sb_TYPE(op) = (type); Sb_INCREF(type); } while(0)
#define SbObject_INIT_VAR(op, type, count) \
    do { SbObject_INIT((op), (type)); Sb_COUNT(op) = (count); } while (0)

/* Defines object allocation/freeing interface */
#define SbObject_Malloc Sb_Malloc
#define SbObject_Realloc Sb_Realloc
#define SbObject_Free Sb_Free

/* Base "object" type. */
extern SbTypeObject *SbObject_Type;

/* New object creation.
   NOTE: Does NOT call `__init__`. */
SbObject *
SbObject_New(SbTypeObject *type);
SbVarObject *
SbObject_NewVar(SbTypeObject *type, Sb_ssize_t count);

/*
 Default method implementations
 */

void
SbObject_DefaultDestroy(SbObject *p);

SbObject *
SbObject_DefaultHash(SbObject *self, SbObject *args, SbObject *kwargs);

SbObject *
SbObject_DefaultGetAttr(SbObject *self, SbObject *args, SbObject *kwargs);
SbObject *
SbObject_DefaultSetAttr(SbObject *self, SbObject *args, SbObject *kwargs);
SbObject *
SbObject_DefaultDelAttr(SbObject *self, SbObject *args, SbObject *kwargs);

SbObject *
SbObject_DefaultStr(SbObject *self, SbObject *args, SbObject *kwargs);


/* Builtin objects */

extern SbObject *Sb_None;

#define Sb_RETURN_NONE \
    do { SbObject *__tmp = Sb_None; Sb_INCREF(__tmp); return __tmp; } while(0)

extern SbObject *Sb_NotImplemented;

typedef SbObject *(*SbUnaryFunc)(SbObject *);
typedef SbObject *(*SbBinaryFunc)(SbObject *, SbObject *);
typedef SbObject *(*SbTernaryFunc)(SbObject *, SbObject *, SbObject *);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_H
