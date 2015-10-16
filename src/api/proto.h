#ifndef __SNAKEBED_PROTO_H
#define __SNAKEBED_PROTO_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 Object protocol.
 */

/* Provides a hash of the given object.
   Returns: the hash if OK, -1 otherwise. */
SbInt_Native_t
SbObject_Hash(SbObject *o);

typedef enum {
    Sb_LT,
    Sb_LE,
    Sb_EQ,
    Sb_NE,
    Sb_GT,
    Sb_GE,
} SbObjectCompareOp;

/* Compare the values of `lhs` and `rhs` using the operation specified by opid.
   Returns: New reference. */
SbObject *
SbObject_Compare(SbObject *lhs, SbObject *rhs, SbObjectCompareOp op);

/* Compare the values of `lhs` and `rhs` using the operation specified by opid.
   Returns: -1 on error, 0 if the result is false, 1 otherwise. */
int
SbObject_CompareBool(SbObject *lhs, SbObject *rhs, SbObjectCompareOp op);
int
SbObject_IsTrue(SbObject *o);
int
SbObject_Not(SbObject *o);

SbObject *
SbObject_Repr(SbObject *o);
SbObject *
SbObject_Str(SbObject *o);

SbObject *
SbObject_GetAttrString(SbObject *o, const char *attr_name);
int
SbObject_SetAttrString(SbObject *o, const char *attr_name, SbObject *v);
int
SbObject_DelAttrString(SbObject *o, const char *attr_name);

/* NOTE: *ObjArgs functions steal references to passed objects. */
SbObject *
SbObject_Call(SbObject *callable, SbObject *args, SbObject *kwargs);
SbObject *
SbObject_CallObjArgs(SbObject *callable, Sb_ssize_t count, ...);
SbObject *
SbObject_CallMethod(SbObject *o, const char *method, SbObject *args, SbObject *kwargs);
SbObject *
SbObject_CallMethodObjArgs(SbObject *o, const char *method, Sb_ssize_t count, ...);

SbObject *
SbObject_GetIter(SbObject *o);

/*
 Number protocol.
 */

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
SbObject *
SbNumber_Lshift(SbObject * lhs, SbObject *rhs);
SbObject *
SbNumber_Rshift(SbObject * lhs, SbObject *rhs);

Sb_ssize_t
SbObject_GetSize(SbObject *o);
SbObject *
SbObject_GetItem(SbObject *o, SbObject *key);
int
SbObject_SetItem(SbObject *o, SbObject *key, SbObject *value);
int
SbObject_DelItem(SbObject *o, SbObject *key);

/*
 Mapping protocol.
 */

Sb_ssize_t
SbMapping_GetSize(SbObject *o);

SbObject *
SbMapping_GetItem(SbObject *o, SbObject *key);
SbObject *
SbMapping_GetItemString(SbObject *o, const char *key);
int
SbMapping_SetItem(SbObject *o, SbObject *key, SbObject *value);
int
SbMapping_SetItemString(SbObject *o, const char *key, SbObject *value);
int
SbMapping_DelItem(SbObject *o, SbObject *key);
int
SbMapping_DelItemString(SbObject *o, const char *key);

/*
 Sequence protocol.
 */

Sb_ssize_t
SbSequence_GetSize(SbObject *o);

/* NOTE: Sequence methods accept integer indices instead of arbitrary key. */
SbObject *
SbSequence_GetItem(SbObject *o, Sb_ssize_t index);
int
SbSequence_SetItem(SbObject *o, Sb_ssize_t index, SbObject *value);
int
SbSequence_DelItem(SbObject *o, Sb_ssize_t index);

SbObject *
SbSequence_GetSlice2(SbObject *o, Sb_ssize_t i1, Sb_ssize_t i2);
int
SbSequence_SetSlice2(SbObject *o, Sb_ssize_t i1, Sb_ssize_t i2, SbObject *values);
int
SbSequence_DelSlice2(SbObject *o, Sb_ssize_t i1, Sb_ssize_t i2);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_PROTO_H
