#include "snakebed.h"

#define SbDict_BUCKET_COUNT 37

typedef struct _SbDictBucketEntry {
    struct _SbDictBucketEntry *e_next;
    long e_hash;
    SbObject *e_key;
    SbObject *e_value;
} SbDictBucketEntry;

/* Define the dict object structure. */
typedef struct _SbDictObject {
    SbObject_HEAD;
    Sb_ssize_t count;
    SbDictBucketEntry *buckets[SbDict_BUCKET_COUNT];
} SbDictObject;

/* Keep the type object here. */
SbTypeObject *SbDict_Type = NULL;

/*
 * C interface implementations
 */

int
SbDict_CheckExact(SbObject *op)
{
    return Sb_TYPE(op) == SbDict_Type;
}

SbObject *
SbDict_New(void)
{
    SbObject *p;

    p = SbObject_New(SbDict_Type);
    if (p) {
        SbDictObject *op = (SbDictObject *)p;
    }
    return p;
}

int 
SbDict_Contains(SbObject *p, SbObject *key)
{
    SbDictObject *op = (SbDictObject *)p;
    SbDictBucketEntry *entry;
    long hash;

    hash = SbObject_Hash(key);
    if (hash == -1) {
        return -1;
    }

    entry = op->buckets[hash % SbDict_BUCKET_COUNT];
    while (entry) {
        if (entry->e_hash == hash && SbObject_CompareBool(entry->e_key, key, Sb_EQ) == 0) {
            return 1;
        }
        entry = entry->e_next;
    }

    return 0;
}

static SbDictBucketEntry *
dict_find_entry_string(SbDictObject *op, const char *key)
{
    long hash;
    SbDictBucketEntry *entry;

    hash = _SbStr_HashString(key, Sb_StrLen(key));
    entry = op->buckets[hash % SbDict_BUCKET_COUNT];
    while (entry) {
        if (entry->e_hash == hash && _SbStr_EqString(entry->e_key, key) == 0) {
            return entry;
        }
        entry = entry->e_next;
    }

    return NULL;
}

SbObject *
SbDict_GetItemString(SbObject *p, const char *key)
{
    SbDictObject *op = (SbDictObject *)p;
    SbDictBucketEntry *entry;

    entry = dict_find_entry_string(op, key);
    if (entry) {
        return entry->e_value;
    }

    return NULL;
}

int
SbDict_SetItemString(SbObject *p, const char *key, SbObject *value)
{
    SbDictObject *op = (SbDictObject *)p;
    SbDictBucketEntry *entry;
    SbDictBucketEntry **bucket;

    Sb_INCREF(value);
    entry = dict_find_entry_string(op, key);
    if (entry) {
        Sb_CLEAR(entry->e_value);
        entry->e_value = value;
        return 0;
    }

    entry = (SbDictBucketEntry *)SbObject_Malloc(sizeof(*entry));
    if (!entry) {
        /* OOM. */
        goto fail0;
    }
    entry->e_key = SbStr_FromString(key);
    if (!entry->e_key) {
        /* OOM? */
        goto fail1;
    }
    entry->e_value = value;
    entry->e_hash = _SbStr_Hash(entry->e_key);
    bucket = &op->buckets[entry->e_hash % SbDict_BUCKET_COUNT];
    entry->e_next = *bucket;
    *bucket = entry;

    return 0;

fail1:
    SbObject_Free(entry);
fail0:
    Sb_DECREF(value);
    return -1;
}

/* Builtins initializer */
int
_SbDict_BuiltinInit()
{
    SbTypeObject *tp;

    tp = SbType_New("dict", NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbDictObject);

    SbDict_Type = tp;
    return 0;
}
