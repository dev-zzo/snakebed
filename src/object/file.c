#include "snakebed.h"
#include "internal.h"

/* Keep the type object here. */
SbTypeObject *SbFile_Type = NULL;

SbObject *
SbFile_New(const char *path, const char *mode)
{
    SbObject *self;
    OSFileHandle_t handle;
    OSError_t status;

    status = Sb_FileOpen(path, mode, &handle);
    if (status != OS_NO_ERROR) {
        SbErr_RaiseIOError(status, NULL);
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
SbFile_FromHandle(OSFileHandle_t handle)
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
    OSError_t status;

    if (!myself->handle) {
        /* raise IOError? */
        return -1;
    }

    status = Sb_FileRead(myself->handle, buffer, count, &transferred);
    if (status != OS_NO_ERROR) {
        SbErr_RaiseIOError(status, NULL);
        return -1;
    }
    return transferred;
}

Sb_ssize_t
SbFile_Write(SbObject *self, const void *buffer, Sb_ssize_t count)
{
    SbFileObject *myself = (SbFileObject *)self;
    Sb_ssize_t transferred;
    OSError_t status;

    if (!myself->handle) {
        /* raise IOError? */
        return -1;
    }

    status = Sb_FileWrite(myself->handle, buffer, count, &transferred);
    if (status != OS_NO_ERROR) {
        SbErr_RaiseIOError(status, NULL);
        return -1;
    }
    return transferred;
}

Sb_ssize_t
SbFile_WriteString(SbObject *self, const char *str)
{
    Sb_ssize_t length;

    length = SbRT_StrLen(str);
    return SbFile_Write(self, str, length);
}

Sb_ssize_t
SbFile_Tell(SbObject *self)
{
    SbFileObject *myself = (SbFileObject *)self;
    Sb_ssize_t result;
    OSError_t status;

    if (!myself->handle) {
        /* raise IOError? */
        return -1;
    }

    status = Sb_FileTell(myself->handle, &result);
    if (status != OS_NO_ERROR) {
        SbErr_RaiseIOError(status, NULL);
        return -1;
    }
    return result;
}

Sb_ssize_t
SbFile_Seek(SbObject *self, Sb_ssize_t offset, int whence)
{
    SbFileObject *myself = (SbFileObject *)self;
    Sb_ssize_t result;
    OSError_t status;

    if (!myself->handle) {
        /* raise IOError? */
        return -1;
    }

    if ((unsigned)whence > 2) {
        SbErr_RaiseWithString(SbExc_ValueError, "`whence` can be {0|1|2} only");
        return -1;
    }

    status = Sb_FileSeek(myself->handle, offset, whence, &result);
    if (status != OS_NO_ERROR) {
        SbErr_RaiseIOError(status, NULL);
        return -1;
    }
    return result;
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
    SbObject *o_result;
    void *buffer;
    Sb_ssize_t maxcount = 16384;
    Sb_ssize_t transferred;

    if (SbArgs_Parse("|i:maxcount", args, kwargs, &maxcount) < 0) {
        return NULL;
    }

    if (maxcount <= 0) {
        SbErr_RaiseWithString(SbExc_ValueError, "'maxcount' must be a positive number");
    }

    buffer = Sb_Malloc(maxcount);
    if (!buffer) {
        return SbErr_NoMemory();
    }

    transferred = SbFile_Read(self, buffer, maxcount);
    if (transferred < 0) {
        Sb_Free(buffer);
        return NULL;
    }

    o_result = SbStr_FromStringAndSize(buffer, transferred);
    Sb_Free(buffer);
    return o_result;
}

static SbObject *
file_write(SbObject *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_data = NULL;
    Sb_ssize_t transferred;

    if (SbArgs_Parse("S:data", args, kwargs, &o_data) < 0) {
        return NULL;
    }

    transferred = SbFile_Write(self, SbStr_AsStringUnsafe(o_data), SbStr_GetSizeUnsafe(o_data));
    if (transferred < 0) {
        return NULL;
    }

    /* TODO: fix this! */
    return SbInt_FromNative(transferred);
}

static SbObject *
file_seek(SbObject *self, SbObject *args, SbObject *kwargs)
{
    Sb_ssize_t offset = 0;
    int whence = 0;

    if (SbArgs_Parse("i:offset|i:whence", args, kwargs, &offset, &whence) < 0) {
        return NULL;
    }

    /* TODO: fix this! */

    return NULL;
}

static SbObject *
file_tell(SbObject *self, SbObject *args, SbObject *kwargs)
{
    /* TODO: fix this! */
    return SbInt_FromNative((long)SbFile_Tell(self));
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
