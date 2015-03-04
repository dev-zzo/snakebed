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
SbObject_DefaultSetAttr(SbObject *self, SbObject *args, SbObject *kwargs);

SbObject *
SbObject_DefaultDelAttr(SbObject *self, SbObject *args, SbObject *kwargs);

SbObject *
SbObject_DefaultStr(SbObject *self, SbObject *args, SbObject *kwargs);


/* Builtin objects */

extern SbObject *Sb_None;

#define Sb_RETURN_NONE \
    Sb_INCREF(Sb_None); \
    return Sb_None

extern SbObject *Sb_NotImplemented;

/*
 Object protocol implementation
 */

/* Provides a hash of the given object.
   Returns: the hash if OK, -1 otherwise. */
long
SbObject_Hash(SbObject *p);

typedef enum {
    Sb_LT,
    Sb_LE,
    Sb_EQ,
    Sb_NE,
    Sb_GT,
    Sb_GE,
} SbObjectCompareOp;

/* Compare the values of o1 and o2 using the operation specified by opid.
   Returns: New reference. */
SbObject *
SbObject_Compare(SbObject *p1, SbObject *p2, SbObjectCompareOp op);

/* Compare the values of o1 and o2 using the operation specified by opid.
   Returns: -1 on error, 0 if the result is false, 1 otherwise. */
int
SbObject_CompareBool(SbObject *p1, SbObject *p2, SbObjectCompareOp op);
int
SbObject_IsTrue(SbObject *p);

SbObject *
SbObject_Repr(SbObject *p);
SbObject *
SbObject_Str(SbObject *p);

SbObject *
SbObject_GetAttrString(SbObject *p, const char *attr_name);
int
SbObject_SetAttrString(SbObject *p, const char *attr_name, SbObject *v);
int
SbObject_DelAttrString(SbObject *p, const char *attr_name);

/* NOTE: *ObjArgs functions steal references to passed objects. */
SbObject *
SbObject_Call(SbObject *callable, SbObject *args, SbObject *kwargs);
SbObject *
SbObject_CallObjArgs(SbObject *callable, Sb_ssize_t count, ...);
SbObject *
SbObject_CallMethod(SbObject *o, const char *method, SbObject *args, SbObject *kwargs);
SbObject *
SbObject_CallMethodObjArgs(SbObject *o, const char *method, Sb_ssize_t count, ...);

/*
 Numeric protocol implementation
 */

typedef SbObject *(*SbUnaryFunc)(SbObject *self);
typedef SbObject *(*SbBinaryFunc)(SbObject *self, SbObject *);
typedef SbObject *(*SbTernaryFunc)(SbObject *self, SbObject *, SbObject *);

/* Unary operations */
SbObject *
SbNumber_Negative(SbObject * rhs);
SbObject *
SbNumber_Positive(SbObject * rhs);
SbObject *
SbNumber_Absolute(SbObject * rhs);
SbObject *
SbNumber_Invert(SbObject * rhs);

/* Binary operations */
SbObject *
SbNumber_Add(SbObject * lhs, SbObject *rhs);
SbObject *
SbNumber_Subtract(SbObject * lhs, SbObject *rhs);
SbObject *
SbNumber_Multiply(SbObject * lhs, SbObject *rhs);
SbObject *
SbNumber_Divide(SbObject * lhs, SbObject *rhs);
SbObject *
SbNumber_FloorDivide(SbObject * lhs, SbObject *rhs);
SbObject *
SbNumber_TrueDivide(SbObject * lhs, SbObject *rhs);
SbObject *
SbNumber_Remainder(SbObject * lhs, SbObject *rhs);
SbObject *
SbNumber_And(SbObject * lhs, SbObject *rhs);
SbObject *
SbNumber_Or(SbObject * lhs, SbObject *rhs);
SbObject *
SbNumber_Xor(SbObject * lhs, SbObject *rhs);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_H
