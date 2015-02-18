#ifndef __SNAKEBED_OBJECT_LIST_H
#define __SNAKEBED_OBJECT_LIST_H
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"

/* Define the list object structure. */
typedef struct _SbListObject {
    SbObject_HEAD;
    Sb_ssize_t allocated;
    SbObject **items;
} SbListObject;

extern SbTypeObject *SbList_Type;

/* Verify the given object is of type list.
   Returns: 1 if true, 0 otherwise. */
int
SbList_CheckExact(SbObject *p);

/* Return a new list object with given size.
   Returns: New reference or NULL on failure. */
SbObject *
SbList_New(Sb_ssize_t length);

/* Construct a new list from the provided arguments.
   Returns: New reference or NULL on failure. */
SbObject *
SbList_Pack(Sb_ssize_t count, ...);

/* Return the list's size.
   WARNING: no type checks are performed. */
#define SbList_GetSizeUnsafe(p) \
    Sb_COUNT(p)

/* Return an item at the given position.
   WARNING: no type/range checks are performed. */
#define SbList_GetItemUnsafe(p, pos) \
    ((SbListObject *)p)->items[pos]

/* Set the list's item at a given position.
   Note: This function "steals" a reference to `o`.
   WARNING: no type/range checks are performed. */
#define SbList_SetItemUnsafe(p, pos, o) \
    ((SbListObject *)p)->items[pos] = o

/* Return the list's size.
   Returns: size if OK, -1 otherwise. */
Sb_ssize_t
SbList_GetSize(SbObject *p);

/* Return an item at the given position.
   Returns: Borrowed reference or NULL on failure. */
SbObject *
SbList_GetItem(SbObject *p, Sb_ssize_t pos);

/* Set the list's item at a given position.
   Note: This function "steals" a reference to `o`.
   Returns: 0 if OK, -1 otherwise. */
int
SbList_SetItem(SbObject *p, Sb_ssize_t pos, SbObject *o);

/* Append an item to the list.
   Returns: 0 if OK, -1 otherwise. */
int
SbList_Append(SbObject *p, SbObject *o);

/* METHODS USED INTERNALLY */

#define SbList_GetAllocated(p) \
    (((SbListObject *)p)->allocated)

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_LIST_H
