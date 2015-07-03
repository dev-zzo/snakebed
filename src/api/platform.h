#ifndef __SNAKEBED_PLATFORM_H
#define __SNAKEBED_PLATFORM_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdlib.h>

#define PLATFORM(x) (PLATFORM_CURRENT == x)
#define PLATFORM_CURRENT 0
#define PLATFORM_WINNT 1
#define PLATFORM_LINUX 2

#include "platform_win32.h"

/* Error handling kludges */

typedef unsigned long OSError_t;

#define OS_NO_ERROR 0

/* NOTE: A pointer to a statically allocated buffer is returned. */
const char *
Sb_StrError(OSError_t error_code);

/* Memory management APIs */

void *
Sb_Malloc(Sb_size_t size);

void *
Sb_Realloc(void *ptr, Sb_size_t new_size);

void
Sb_Free(void *ptr);

/* File operations abstraction */

/* NOTE: Currently, this fails to handle files larger than 2Gb. */

typedef void *OSFileHandle_t;

OSError_t
Sb_FileOpen(const char *path, const char *mode, OSFileHandle_t *handle);

OSError_t
Sb_FileRead(OSFileHandle_t handle, void *buffer, Sb_ssize_t count, Sb_ssize_t *read);

OSError_t
Sb_FileWrite(OSFileHandle_t handle, const void *buffer, Sb_ssize_t count, Sb_ssize_t *written);

OSError_t
Sb_FileTell(OSFileHandle_t handle, Sb_ssize_t *offset);

OSError_t
Sb_FileSeek(OSFileHandle_t handle, Sb_ssize_t offset, int whence, Sb_ssize_t *new_pos);

void
Sb_FileClose(OSFileHandle_t handle);

/* Standard input/output/error */

OSFileHandle_t
Sb_GetStdInHandle(void);
OSFileHandle_t
Sb_GetStdOutHandle(void);
OSFileHandle_t
Sb_GetStdErrHandle(void);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_PLATFORM_H
