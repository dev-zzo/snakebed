#ifndef __SNAKEBED_OBJECT_FILE_H
#define __SNAKEBED_OBJECT_FILE_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SbFileObject {
    SbObject_HEAD;
    void *handle;
    Sb_size_t last_error;
} SbFileObject;

extern SbTypeObject *SbFile_Type;

SbObject *
SbFile_New(const char *path, const char *mode);

SbObject *
SbFile_FromHandle(void *handle);

Sb_ssize_t
SbFile_Read(SbObject *self, void *buffer, Sb_ssize_t count);

Sb_ssize_t
SbFile_Write(SbObject *self, const void *buffer, Sb_ssize_t count);

Sb_size_t
SbFile_Tell(SbObject *self);

Sb_size_t
SbFile_Seek(SbObject *self, Sb_ssize_t offset, int whence);

void
SbFile_Close(SbObject *self);


#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OBJECT_FILE_H
