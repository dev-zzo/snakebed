#include "snakebed.h"
#include "internal.h"

/*
 * Dictinary object implementation
 */

/* Dictionary implementation details discussion

The implementation is your good old hash table. Since the hash function 
is believed to be "random enough", the bucket count is chosen to be
a power of 2, to avoid high cost modulo when looking up the bucket.

Memory costs estimate (32-bit systems):
- Base object: sizeof(void *) * (2 + 1 + BUCKET_COUNT) = sizeof(void *) * 35 = 140
- Each entry: sizeof(void *) * 4 = 16

*/

#define BUCKET_COUNT 32U

typedef struct _bucket_entry {
    struct _bucket_entry *e_next;
    long e_hash;
    SbObject *e_key;
    SbObject *e_value;
} bucket_entry;

/* Define the dict object structure. */
struct _SbDictObject {
    SbObject_HEAD;
    Sb_ssize_t count;
    bucket_entry *buckets[BUCKET_COUNT];
};

/* Keep the type object here. */
SbTypeObject *SbDict_Type = NULL;

/*
 * C interface implementations
 */

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

Sb_ssize_t
SbDict_GetSize(SbObject *p)
{
#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbDict_CheckExact(p)) {
        SbErr_RaiseWithString(SbExc_SystemError, "non-dict object passed to a dict method");
        return -1;
    }
#endif

    return SbDict_GetSizeUnsafe(p);
}

Sb_ssize_t
SbDict_GetSizeUnsafe(SbObject *p)
{
    return ((SbDictObject *)p)->count;
}

void
_SbDict_Clear(SbDictObject *myself)
{
    Sb_ssize_t bucket;

    for (bucket = 0; bucket < BUCKET_COUNT; ++bucket) {
        bucket_entry *entry;

        entry = myself->buckets[bucket];
        while (entry) {
            myself->buckets[bucket] = entry->e_next;
            Sb_DECREF(entry->e_key);
            Sb_DECREF(entry->e_value);
            entry = entry->e_next;
        }
    }

    myself->count = 0;
}

void
SbDict_Clear(SbObject *p)
{
#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbDict_CheckExact(p)) {
        SbErr_RaiseWithString(SbExc_SystemError, "non-dict object passed to a dict method");
        return;
    }
#endif

    _SbDict_Clear((SbDictObject *)p);
}

static bucket_entry **
dict_bucket_ptr(SbDictObject *op, long hash)
{
    return &op->buckets[((unsigned long)hash) % BUCKET_COUNT];
}

static SbObject *
dict_getitem_common(SbDictObject *myself, long hash, void *key, int (*cmp)(SbObject *e_key, void *key))
{
    bucket_entry *entry;

    entry = *dict_bucket_ptr(myself, hash);
    while (entry) {
        if (entry->e_hash == hash) {
            if (cmp(entry->e_key, key)) {
                return entry->e_value;
            }
        }
        entry = entry->e_next;
    }

    return NULL;
}

static int
dict_getitemstring_cmp(SbObject *e_key, void *key)
{
    return SbStr_CheckExact(e_key) && _SbStr_EqString(e_key, (const char *)key) == 1;
}

SbObject *
SbDict_GetItemString(SbObject *p, const char *key)
{
    SbDictObject *myself = (SbDictObject *)p;
    long hash;

#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbDict_CheckExact(p)) {
        SbErr_RaiseWithString(SbExc_SystemError, "non-dict object passed to a dict method");
        return NULL;
    }
#endif

    hash = _SbStr_HashString(key, SbRT_StrLen(key));
    return dict_getitem_common(myself, hash, (void *)key, dict_getitemstring_cmp);
}

static int
dict_getitem_cmp(SbObject *e_key, void *key)
{
    return SbObject_CompareBool(e_key, (SbObject *)key, Sb_EQ) == 1;
}

SbObject *
SbDict_GetItem(SbObject *p, SbObject *key)
{
    SbDictObject *myself = (SbDictObject *)p;
    long hash;

#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbDict_CheckExact(p)) {
        SbErr_RaiseWithString(SbExc_SystemError, "non-dict object passed to a dict method");
        return NULL;
    }
#endif

    hash = SbObject_Hash(key);
    if (hash != -1) {
        return dict_getitem_common(myself, hash, key, dict_getitem_cmp);
    }
    return NULL;
}


int
SbDict_SetItemString(SbObject *p, const char *key, SbObject *value)
{
    SbDictObject *myself = (SbDictObject *)p;
    bucket_entry *entry;
    bucket_entry **bucket;
    long hash;

#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbDict_CheckExact(p)) {
        SbErr_RaiseWithString(SbExc_SystemError, "non-dict object passed to a dict method");
        return -1;
    }
#endif

    Sb_INCREF(value);

    hash = _SbStr_HashString(key, SbRT_StrLen(key));
    bucket = dict_bucket_ptr(myself, hash);
    entry = *bucket;
    while (entry) {
        if (entry->e_hash == hash) {
            SbObject *o_key = entry->e_key;

            if (SbStr_CheckExact(o_key) && _SbStr_EqString(o_key, key) == 1) {
                Sb_CLEAR(entry->e_value);
                entry->e_value = value;
                return 0;
            }
        }
        entry = entry->e_next;
    }

    entry = (bucket_entry *)Sb_Malloc(sizeof(*entry));
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
    entry->e_hash = hash;
    entry->e_next = *bucket;
    *bucket = entry;
    myself->count++;
    return 0;

fail1:
    Sb_Free(entry);
fail0:
    return -1;
}

int
SbDict_SetItem(SbObject *p, SbObject *key, SbObject *value)
{
    SbDictObject *myself = (SbDictObject *)p;
    bucket_entry *entry;
    bucket_entry **bucket;
    long hash;

#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbDict_CheckExact(p)) {
        SbErr_RaiseWithString(SbExc_SystemError, "non-dict object passed to a dict method");
        return -1;
    }
#endif

    Sb_INCREF(value);

    hash = SbObject_Hash(key);
    if (hash == -1) {
        return -1;
    }
    bucket = dict_bucket_ptr(myself, hash);
    entry = *bucket;
    while (entry) {
        if (entry->e_hash == hash) {
            if (SbObject_CompareBool(entry->e_key, key, Sb_EQ) == 1) {
                Sb_CLEAR(entry->e_value);
                entry->e_value = value;
                return 0;
            }
        }
        entry = entry->e_next;
    }

    entry = (bucket_entry *)Sb_Malloc(sizeof(*entry));
    if (!entry) {
        SbErr_NoMemory();
        return -1;
    }
    Sb_INCREF(key);
    entry->e_key = key;
    entry->e_value = value;
    entry->e_hash = hash;
    entry->e_next = *bucket;
    *bucket = entry;
    myself->count++;
    return 0;
}


int
SbDict_DelItemString(SbObject *p, const char *key)
{
    SbDictObject *myself = (SbDictObject *)p;
    bucket_entry *entry;
    bucket_entry **bucket;
    long hash;
    bucket_entry *prev_entry;

#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbDict_CheckExact(p)) {
        SbErr_RaiseWithString(SbExc_SystemError, "non-dict object passed to a dict method");
        return -1;
    }
#endif

    hash = _SbStr_HashString(key, SbRT_StrLen(key));
    bucket = dict_bucket_ptr(myself, hash);
    entry = *bucket;
    /* This exploits the fact that e_next is located at offset zero in entry. */
    prev_entry = (bucket_entry *)bucket;
    while (entry) {
        if (entry->e_hash == hash) {
            SbObject *o_key = entry->e_key;

            if (SbStr_CheckExact(o_key) && _SbStr_EqString(o_key, key) == 1) {
                prev_entry->e_next = entry->e_next;
                /* Safe to decref -- the entry is no longer in. */
                Sb_DECREF(entry->e_key);
                Sb_DECREF(entry->e_value);
                myself->count--;
                return 0;
            }
        }
        prev_entry = entry;
        entry = entry->e_next;
    }

    SbErr_RaiseWithString(SbExc_KeyError, key);
    return -1;
}

int
SbDict_DelItem(SbObject *p, SbObject *key)
{
    SbDictObject *myself = (SbDictObject *)p;
    bucket_entry *entry;
    bucket_entry **bucket;
    long hash;
    bucket_entry *prev_entry;

#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbDict_CheckExact(p)) {
        SbErr_RaiseWithString(SbExc_SystemError, "non-dict object passed to a dict method");
        return -1;
    }
#endif

    hash = SbObject_Hash(key);
    if (hash == -1) {
        return -1;
    }
    bucket = dict_bucket_ptr(myself, hash);
    entry = *bucket;
    /* This exploits the fact that e_next is located at offset zero in entry. */
    prev_entry = (bucket_entry *)bucket;
    while (entry) {
        if (entry->e_hash == hash) {
            if (SbObject_CompareBool(entry->e_key, key, Sb_EQ) == 1) {
                prev_entry->e_next = entry->e_next;
                /* Safe to decref -- the entry is no longer in. */
                Sb_DECREF(entry->e_key);
                Sb_DECREF(entry->e_value);
                myself->count--;
                return 0;
            }
        }
        prev_entry = entry;
        entry = entry->e_next;
    }

    SbErr_RaiseWithObject(SbExc_KeyError, key);
    return -1;
}


typedef struct _dict_iteration_state {
    Sb_ssize_t bucket;
    bucket_entry *entry;
} dict_iteration_state;

int
SbDict_Next(SbObject *p, Sb_ssize_t *state, SbObject **key, SbObject **value)
{
    SbDictObject *myself = (SbDictObject *)p;
    dict_iteration_state *s;
    Sb_ssize_t bucket;
    bucket_entry *entry;

#if SUPPORTS(BUILTIN_TYPECHECKS)
    if (!SbDict_CheckExact(p)) {
        SbErr_RaiseWithString(SbExc_SystemError, "non-dict object passed to a dict method");
        return -1;
    }
#endif

    /* Trivial case */
    if (SbDict_GetSizeUnsafe(p) == 0) {
        goto iteration_end0;
    }

    if (*state == 0) {
        s = (dict_iteration_state *)Sb_Malloc(sizeof(*s));
        if (!s) {
            SbErr_NoMemory();
            return -1;
        }
        *state = (Sb_ssize_t)s;

        bucket = -1;
        entry = NULL;
        /* It is known there is at least one entry. */
        goto search_buckets;
    }

    s = (dict_iteration_state *)*state;
    bucket = s->bucket;
    entry = s->entry->e_next;

search_buckets:
    if (!entry) {
        do {
            ++bucket;
            if (bucket == BUCKET_COUNT) {
                goto iteration_end;
            }
        } while (myself->buckets[bucket] == NULL);
        entry = myself->buckets[bucket];
    }

    s->bucket = bucket;
    s->entry = entry;
    *key = entry->e_key;
    *value = entry->e_value;
    return 1;

iteration_end:
    Sb_Free(s);
    *state = 0;

iteration_end0:
    *key = NULL;
    *value = NULL;
    return 0;
}

int
SbDict_Merge(SbObject *dst, SbObject *src, int update)
{
    Sb_ssize_t bucket;
    SbDictObject *_src = (SbDictObject *)src;

    for (bucket = 0; bucket < BUCKET_COUNT; ++bucket) {
        bucket_entry *src_entry;

        for (src_entry = _src->buckets[bucket]; src_entry; src_entry = src_entry->e_next) {
            SbObject *o;

            o = SbDict_GetItem(dst, src_entry->e_key);
            if (!o || update) {
                if (SbDict_SetItem(dst, src_entry->e_key, src_entry->e_value) < 0) {
                    return -1;
                }
            }
        }
    }
    return 0;
}

SbObject *
SbDict_Copy(SbObject *p)
{
    SbObject *dict;

    dict = SbDict_New();
    if (!dict) {
        goto fail0;
    }

    if (SbDict_Merge(dict, p, 1) < 0) {
        goto fail1;
    }

    return dict;

fail1:
    Sb_DECREF(dict);
fail0:
    return NULL;
}


static SbObject *
dict_len(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return SbInt_FromNative(SbDict_GetSizeUnsafe(self));
}

static SbObject *
dict_getitem(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *key;
    SbObject *result;

    if (SbArgs_Parse("O:key", args, kwargs, &key) < 0) {
        return NULL;
    }
    result = SbDict_GetItem(self, key);
    if (result) {
        Sb_INCREF(result);
    }
    return result;
}

static SbObject *
dict_setitem(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *key;
    SbObject *value;
    int result;

    if (SbArgs_Parse("O:key,O:value", args, kwargs, &key, &value) < 0) {
        return NULL;
    }
    result = SbDict_SetItem(self, key, value);
    if (result < 0) {
        return NULL;
    }
    Sb_RETURN_NONE;
}

static SbObject *
dict_delitem(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *key;
    int result;

    if (SbArgs_Parse("O:key", args, kwargs, &key) < 0) {
        return NULL;
    }
    result = SbDict_DelItem(self, key);
    if (result < 0) {
        return NULL;
    }
    Sb_RETURN_NONE;
}


typedef SbObject *(*iter_callback)(SbObject *key, SbObject *value);

static SbObject *
iter_new(SbObject *dict, iter_callback cb);

static SbObject *
iterkeys_callback(SbObject *key, SbObject *value)
{
    Sb_INCREF(key);
    return key;
}

static SbObject *
dict_iterkeys(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return iter_new(self, iterkeys_callback);
}

static SbObject *
itervalues_callback(SbObject *key, SbObject *value)
{
    Sb_INCREF(value);
    return value;
}

static SbObject *
dict_itervalues(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return iter_new(self, itervalues_callback);
}

static SbObject *
iteritems_callback(SbObject *key, SbObject *value)
{
    return SbTuple_Pack(2, key, value);
}

static SbObject *
dict_iteritems(SbObject *self, SbObject *args, SbObject *kwargs)
{
    return iter_new(self, iteritems_callback);
}


static const SbCMethodDef dict_methods[] = {
    { "__len__", dict_len },
    { "__getitem__", dict_getitem },
    { "__setitem__", dict_setitem },
    { "__delitem__", dict_delitem },

    { "iterkeys", dict_iterkeys },
    { "itervalues", dict_itervalues },
    { "iteritems", dict_iteritems },

    /* Sentinel */
    { NULL, NULL },
};

int
_Sb_TypeInit_Dict()
{
    SbTypeObject *tp;

    tp = SbType_New("dict", NULL, NULL);
    if (!tp) {
        return -1;
    }

    tp->tp_basicsize = sizeof(SbDictObject);
    tp->tp_destroy = (SbDestroyFunc)dict_destroy;

    SbDict_Type = tp;
    return 0;
}

static int
_Sb_TypeInit_DictIter();

int
_Sb_TypeInit2_Dict()
{
    SbObject *dict;

    dict = _SbType_BuildMethodDict(dict_methods);
    if (!dict) {
        return -1;
    }
    SbDict_Type->tp_dict = dict;

    return _Sb_TypeInit_DictIter();
}

/*
 * Dictinary iterator object
 */

typedef struct _SbDictIterObject {
    SbObject_HEAD;
    SbObject *dict;
    Sb_ssize_t state;
    iter_callback cb;
} SbDictIterObject;

static SbTypeObject *SbDictIter_Type = NULL;

static SbObject *
iter_new(SbObject *dict, iter_callback cb)
{
    SbObject *self;

    self = SbObject_New(SbDictIter_Type);
    if (self) {
        SbDictIterObject *myself = (SbDictIterObject *)self;
        Sb_INCREF(dict);
        myself->dict = dict;
        myself->state = 0;
        myself->cb = cb;
    }
    return self;
}

static void
iter_destroy(SbObject *self)
{
    SbDictIterObject *myself = (SbDictIterObject *)self;

    Sb_CLEAR(myself->dict);
}

static SbObject *
iter_self(SbObject *self, SbObject *args, SbObject *kwargs)
{
    Sb_INCREF(self);
    return self;
}

static SbObject *
iter_next(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbDictIterObject *myself = (SbDictIterObject *)self;
    SbObject *key;
    SbObject *value;

    switch (SbDict_Next(myself->dict, &myself->state, &key, &value)) {
    case 0:
        return SbErr_NoMoreItems();
    case 1:
        return myself->cb(key, value);
    default:
        return NULL;
    }
}

static const SbCMethodDef iter_methods[] = {
    { "__iter__", iter_self },
    { "next", iter_next },
    /* Sentinel */
    { NULL, NULL },
};

static int
_Sb_TypeInit_DictIter()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("<dictiterator>", NULL, iter_methods, sizeof(SbDictIterObject));
    if (!tp) {
        return -1;
    }
    tp->tp_destroy = (SbDestroyFunc)iter_destroy;

    SbDictIter_Type = tp;
    return 0;
}
