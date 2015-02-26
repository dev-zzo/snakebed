#ifndef __SNAKEBED_PLATFORM_H
#define __SNAKEBED_PLATFORM_H
#ifdef __cplusplus
extern "C" {
#endif

#include "platform_win32.h"

void *
Sb_Malloc(Sb_size_t size);

void *
Sb_Realloc(void *ptr, Sb_size_t new_size);

void
Sb_Free(void *ptr);

/* File operations abstraction */

/* Open a file object.
   Returns: A handle to the opened object or NULL on error. */
void *
Sb_FileOpen(const char *path, const char *mode);

Sb_ssize_t
Sb_FileRead(void *handle, void *buffer, Sb_ssize_t count);

Sb_ssize_t
Sb_FileWrite(void *handle, const void *buffer, Sb_ssize_t count);

Sb_size_t
Sb_FileTell(void *handle);

Sb_size_t
Sb_FileSeek(void *handle, Sb_ssize_t offset, int whence);

void
Sb_FileClose(void *handle);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_PLATFORM_H
