#include "../bstrap.h"
#include <windows.h>

static DWORD last_file_error;

OSError_t
Sb_FileOpen(const char *path, const char *mode, OSFileHandle_t *handle)
{
    HANDLE h;
    DWORD access = 0;
    DWORD create_disp;

    if (mode[0] == 'r') {
        access = GENERIC_READ;
        create_disp = OPEN_EXISTING;
    }
    else if (mode[0] == 'w') {
        access = GENERIC_READ | GENERIC_WRITE;
        create_disp = OPEN_ALWAYS;
    }

    h = CreateFileA(
        path,
        access,
        FILE_SHARE_READ,
        NULL,
        create_disp,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (h == INVALID_HANDLE_VALUE) {
        return (OSError_t)GetLastError();
    }

    *handle = (OSFileHandle_t)h;
    return OS_NO_ERROR;
}

OSError_t
Sb_FileRead(OSFileHandle_t handle, void *buffer, Sb_ssize_t count, Sb_ssize_t *read)
{
    DWORD transferred;

    if (ReadFile((HANDLE)handle, buffer, count, &transferred, NULL)) {
        if (read) {
            *read = transferred;
        }
        return OS_NO_ERROR;
    }
    return (OSError_t)GetLastError();
}

OSError_t
Sb_FileWrite(OSFileHandle_t handle, const void *buffer, Sb_ssize_t count, Sb_ssize_t *written)
{
    DWORD transferred;

    if (WriteFile((HANDLE)handle, buffer, count, &transferred, NULL)) {
        if (written) {
            *written = transferred;
        }
        return OS_NO_ERROR;
    }
    return (OSError_t)GetLastError();
}

OSError_t
Sb_FileTell(OSFileHandle_t handle, Sb_ssize_t *offset)
{
    return Sb_FileSeek(handle, 0, FILE_CURRENT, offset);
}

OSError_t
Sb_FileSeek(OSFileHandle_t handle, Sb_ssize_t offset, int whence, Sb_ssize_t *new_pos)
{
    LARGE_INTEGER distance;
    LARGE_INTEGER pos;

    distance.QuadPart = offset;
    if (SetFilePointerEx((HANDLE)handle, distance, &pos, whence) < 0) {
        return (OSError_t)GetLastError();
    }

    if (new_pos) {
        *new_pos = (Sb_ssize_t)pos.QuadPart;
    }
    return OS_NO_ERROR;
}

void
Sb_FileClose(OSFileHandle_t handle)
{
    CloseHandle((HANDLE)handle);
}

OSFileHandle_t
Sb_GetStdInHandle(void)
{
    OSFileHandle_t dupHandle;

    DuplicateHandle(
        GetCurrentProcess(), GetStdHandle(STD_INPUT_HANDLE),
        GetCurrentProcess(), &dupHandle,
        0, TRUE, DUPLICATE_SAME_ACCESS);
    return dupHandle;
}

OSFileHandle_t
Sb_GetStdOutHandle(void)
{
    OSFileHandle_t dupHandle;

    DuplicateHandle(
        GetCurrentProcess(), GetStdHandle(STD_OUTPUT_HANDLE),
        GetCurrentProcess(), &dupHandle,
        0, TRUE, DUPLICATE_SAME_ACCESS);
    return dupHandle;
}

OSFileHandle_t
Sb_GetStdErrHandle(void)
{
    OSFileHandle_t dupHandle;

    DuplicateHandle(
        GetCurrentProcess(), GetStdHandle(STD_ERROR_HANDLE),
        GetCurrentProcess(), &dupHandle,
        0, TRUE, DUPLICATE_SAME_ACCESS);
    return dupHandle;
}
