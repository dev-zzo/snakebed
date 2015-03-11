#include "snakebed.h"

/* Keep the type object here. */
SbTypeObject *SbFile_Type = NULL;

SbObject *
SbFile_New(const char *path, const char *mode)
{
    SbObject *self;
    void *handle;

    handle = Sb_FileOpen(path, mode);
    if (!handle) {
        /* raise IOError? */
        return NULL;
    }

    self = SbFile_FromHandle(handle);
    if (!self) {
        Sb_FileClose(handle);
    }

    return self;
}

static void
file_destroy(SbFileObject *myself)
{
    if (myself->handle) {
        Sb_FileClose(myself->handle);
    }
    SbObject_DefaultDestroy((SbObject *)myself);
}

SbObject *
SbFile_FromHandle(void *handle)
{
    SbObject *self;

    self = SbObject_New(SbFile_Type);
    if (self) {
        SbFileObject *f = (SbFileObject *)self;

        f->handle = handle;
    }

    return self;
}

Sb_ssize_t
SbFile_Read(SbObject *self, void *buffer, Sb_ssize_t count)
{
    SbFileObject *myself = (SbFileObject *)self;
    Sb_ssize_t transferred;

    if (!myself->handle) {
        /* raise IOError? */
        return -1;
    }

    transferred = Sb_FileRead(myself->handle, buffer, count);
    if (transferred < 0) {
        /* raise IOError? */
    }
    return transferred;
}

Sb_ssize_t
SbFile_Write(SbObject *self, const void *buffer, Sb_ssize_t count)
{
    SbFileObject *myself = (SbFileObject *)self;
    Sb_ssize_t transferred;

    if (!myself->handle) {
        /* raise IOError? */
        return -1;
    }

    transferred = Sb_FileWrite(myself->handle, buffer, count);
    if (transferred < 0) {
        /* raise IOError? */
    }
    return transferred;
}

Sb_size_t
SbFile_Tell(SbObject *self)
{
    SbFileObject *myself = (SbFileObject *)self;

    if (!myself->handle) {
        /* raise IOError? */
        return -1;
    }

    return Sb_FileTell(myself->handle);
}

Sb_size_t
SbFile_Seek(SbObject *self, Sb_ssize_t offset, int whence)
{
    SbFileObject *myself = (SbFileObject *)self;

    if (!myself->handle) {
        /* raise IOError? */
        return -1;
    }
    if ((unsigned)whence > 2) {
        SbErr_RaiseWithString(SbErr_ValueError, "`whence` can be {0|1|2} only");
        return -1;
    }

    return Sb_FileSeek(myself->handle, offset, whence);
}

void
SbFile_Close(SbObject *self)
{
    SbFileObject *myself = (SbFileObject *)self;

    if (myself->handle) {
        Sb_FileClose(myself->handle);
        myself->handle = NULL;
    }
}

static SbObject *
file_read(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_maxcount = NULL;
    SbObject *o_result;
    void *buffer;
    Sb_size_t maxcount;
    Sb_ssize_t transferred;

    if (SbTuple_Unpack(args, 0, 1, &o_maxcount) < 0) {
        return NULL;
    }

    if (o_maxcount) {
        /* TODO: fix this! */
        maxcount = SbInt_AsLong(o_maxcount);
    }
    else {
        /* Some sensible default?
           The spec says "all the file till the end", but... */
        maxcount = 16384;
    }

    if (maxcount <= 0) {
        SbErr_RaiseWithString(SbErr_ValueError, "`maxcount` must be a positive number");
    }

    buffer = Sb_Malloc(maxcount);
    if (!buffer) {
        return NULL;
    }

    transferred = SbFile_Read(self, buffer, maxcount);
    if (transferred < 0) {
        Sb_Free(buffer);
        return NULL;
    }

    o_result = SbStr_FromStringAndSize(buffer, transferred);
    Sb_Free(buffer);
    if (!o_result) {
        return NULL;
    }

    return o_result;
}

static SbObject *
file_write(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_data = NULL;
    Sb_ssize_t transferred;

    if (SbTuple_Unpack(args, 1, 1, &o_data) < 0) {
        return NULL;
    }
    if (!SbStr_CheckExact(o_data)) {
        SbErr_RaiseWithString(SbErr_TypeError, "expected a str");
        return NULL;
    }

    transferred = SbFile_Write(self, SbStr_AsStringUnsafe(o_data), SbStr_GetSizeUnsafe(o_data));
    if (transferred < 0) {
        return NULL;
    }

    /* TODO: fix this! */
    return SbInt_FromLong(transferred);
}

static SbObject *
file_seek(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_offset = NULL;
    SbObject *o_whence = NULL;
    Sb_ssize_t offset;
    int whence = 0;

    if (SbTuple_Unpack(args, 1, 2, &o_offset, &o_whence) < 0) {
        return NULL;
    }

    /* TODO: fix this! */
    offset = SbInt_AsLong(o_offset);
    if (o_whence) {
        whence = SbInt_AsLong(o_whence);
    }

    return NULL;
}

static SbObject *
file_tell(SbObject *self, SbObject *args, SbObject *kwargs)
{
    /* TODO: fix this! */
    return SbInt_FromLong((long)SbFile_Tell(self));
}

static SbObject *
file_close(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbFile_Close(self);
    Sb_RETURN_NONE;
}

/* Type initializer */

static const SbCMethodDef file_methods[] = {
    { "read", file_read },
    { "write", file_write },
    { "seek", file_seek },
    { "tell", file_tell },
    { "close", file_close },
    /* Sentinel */
    { NULL, NULL },
};

int
_SbFile_TypeInit()
{
    SbTypeObject *tp;

    tp = _SbType_FromCDefs("file", NULL, file_methods, sizeof(SbFileObject));
    if (!tp) {
        return -1;
    }

    tp->tp_destroy = (SbDestroyFunc)file_destroy;

    SbFile_Type = tp;
    return 0;
}
