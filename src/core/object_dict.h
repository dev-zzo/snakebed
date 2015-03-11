#ifndef __SNAKEBED_OBJECT_DICT_H
#define __SNAKEBED_OBJECT_DICT_H
#ifdef __cplusplus
extern "C" {
#endif

struct _SbDictObject;
typedef struct _SbDictObject SbDictObject;

extern SbTypeObject *SbDict_Type;

/* Verify the given object is of type dict.
   Returns: 1 if true, 0 otherwise. */
#define SbDict_CheckExact(p) \
    (Sb_TYPE(p) == SbDict_Type)

/* Create a new dictionary object.
   Returns: New reference or NULL on failure. */
SbObject *
SbDict_New(void);

/* Remove all elements from the dict. */
void
SbDict_Clear(SbObject *p);
void
_SbDict_Clear(SbDictObject *op);

/* Test whether the dict containt the key.
   Returns: 0/1 if OK, -1 otherwise. */
int 
SbDict_Contains(SbObject *p, SbObject *key);

/* Get the keys count in the dict. */
Sb_ssize_t
SbDict_GetSize(SbObject *p);

/* Get the keys count in the dict.
   WARNING: no type checks are performed. */
Sb_ssize_t
SbDict_GetSizeUnsafe(SbObject *p);

#if 0
/* Return the object that has a given key.
   Returns: Borrowed reference. */
SbObject *
SbDict_GetItem(SbObject *p, SbObject *key);

/* Insert the object at the given key.
   Returns: 0 if OK, -1 otherwise. */
int
SbDict_SetItem(SbObject *p, SbObject *key, SbObject *value);

/* Deletes the object at the given key.
   Returns: 0 if OK, -1 otherwise. */
int
SbDict_DelItem(SbObject *p, SbObject *key);
#endif

/* Hacked up API to avoid rich comparison when keys are strings. */

/* Return the object that has a given key.
   Returns: Borrowed reference. */
SbObject *
SbDict_GetItemString(SbObject *p, const char *key);

/* Insert the object at the given key.
   Returns: 0 if OK, -1 otherwise. */
int
SbDict_SetItemString(SbObject *p, const char *key, SbObject *value);

/* Deletes the object at the given key.
   Returns: 0 if OK, -1 otherwise. */
int
SbDict_DelItemString(SbObject *p, const char *key);


/* Iterates through all key-value pairs in the dict.
   `state` must be initialized to 0 to start iteration.
   Note: References in `key` and `value` are borrowed.
   Returns: 0 when finished. */
int
SbDict_Next(SbObject *p, Sb_ssize_t *state, SbObject **key, SbObject **value);

/* Build a shallow copy of the given dictionary.
   Returns: New reference. */
SbObject *
SbDict_Copy(SbObject *p);

/* Update the `dst` with items from `src`.
   Returns: 0 if OK, -1 otherwise. */
int
SbDict_Merge(SbObject *dst, SbObject *src, int update);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_DICT_H
