#include "snakebed.h"

#define SbDict_BUCKET_COUNT 37U

typedef struct _SbDictBucketEntry {
    struct _SbDictBucketEntry *e_next;
    long e_hash;
    SbObject *e_key;
    SbObject *e_value;
} SbDictBucketEntry;

/* Define the dict object structure. */
struct _SbDictObject {
    SbObject_HEAD;
    Sb_ssize_t count;
    SbDictBucketEntry *buckets[SbDict_BUCKET_COUNT];
};

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

Sb_ssize_t
SbDict_GetSize(SbObject *p)
{
    if (!SbDict_CheckExact(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-dict object passed to a dict method");
        return -1;
    }

    return SbDict_GetSizeUnsafe(p);
}

Sb_ssize_t
SbDict_GetSizeUnsafe(SbObject *p)
{
    return ((SbDictObject *)p)->count;
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

static void
dict_destroy(SbDictObject *self)
{
    _SbDict_Clear(self);
    SbObject_DefaultDestroy((SbObject *)self);
}

void
_SbDict_Clear(SbDictObject *op)
{
    int bucket_index;

    for (bucket_index = 0; bucket_index < SbDict_BUCKET_COUNT; ++bucket_index) {
        SbDictBucketEntry *entry;

        entry = op->buckets[bucket_index];
        while (entry) {
            op->buckets[bucket_index] = entry->e_next;
            Sb_DECREF(entry->e_key);
            Sb_DECREF(entry->e_value);
            entry = entry->e_next;
        }
    }

    op->count = 0;
}

void
SbDict_Clear(SbObject *p)
{
    if (!SbDict_CheckExact(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-dict object passed to a dict method");
        return;
    }
    _SbDict_Clear((SbDictObject *)p);
}

static SbDictBucketEntry **
dict_bucket_ptr(SbDictObject *op, long hash)
{
    return &op->buckets[((unsigned long)hash) % SbDict_BUCKET_COUNT];
}

int 
SbDict_Contains(SbObject *p, SbObject *key)
{
    SbDictObject *op = (SbDictObject *)p;
    SbDictBucketEntry *entry;
    long hash;

    if (!SbDict_CheckExact(p)) {
        SbErr_RaiseWithString(SbErr_SystemError, "non-dict object passed to a dict method");
        return -1;
    }

    hash = SbObject_Hash(key);
    if (hash == -1) {
        return -1;
    }

    entry = *dict_bucket_ptr(op, hash);
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
    entry = *dict_bucket_ptr(op, hash);
    while (entry) {
        if (entry->e_hash == hash && _SbStr_EqString(entry->e_key, key) == 1) {
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
        SbErr_NoMemory();
        goto fail0;
    }
    entry->e_key = SbStr_FromString(key);
    if (!entry->e_key) {
        SbErr_NoMemory();
        goto fail1;
    }
    entry->e_value = value;
    entry->e_hash = _SbStr_Hash(entry->e_key);
    bucket = dict_bucket_ptr(op, entry->e_hash);
    entry->e_next = *bucket;
    *bucket = entry;

    op->count++;

    return 0;

fail1:
    SbObject_Free(entry);
fail0:
    Sb_DECREF(value);
    return -1;
}

int
SbDict_DelItemString(SbObject *p, const char *key)
{
    SbDictObject *op = (SbDictObject *)p;
    long hash;
    SbDictBucketEntry **bucket;
    SbDictBucketEntry *entry;
    SbDictBucketEntry *prev_entry;

    hash = _SbStr_HashString(key, Sb_StrLen(key));
    bucket = dict_bucket_ptr(op, hash);
    entry = *bucket;
    /* This exploits the fact that e_next is located at offset zero in entry. */
    prev_entry = (SbDictBucketEntry *)bucket;
    while (entry) {
        if (entry->e_hash == hash && _SbStr_EqString(entry->e_key, key) == 1) {
            prev_entry->e_next = entry->e_next;
            /* Safe to decref -- the entry is no longer in. */
            Sb_DECREF(entry->e_key);
            Sb_DECREF(entry->e_value);
            op->count--;
            return 0;
        }
        prev_entry = entry;
        entry = entry->e_next;
    }

    SbErr_RaiseWithString(SbErr_KeyError, key);
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
    tp->tp_destroy = (destructor)dict_destroy;

    SbDict_Type = tp;
    return 0;
}
