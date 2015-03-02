#include "../bstrap.h"
#include <windows.h>

static DWORD last_file_error;

void *
Sb_FileOpen(const char *path, const char *mode)
{
    HANDLE handle;
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

    handle = CreateFileA(
        path,
        access,
        FILE_SHARE_READ,
        NULL,
        create_disp,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    return (void *)handle;
}

Sb_ssize_t
Sb_FileRead(void *handle, void *buffer, Sb_ssize_t count)
{
    DWORD transferred;

    if (!ReadFile((HANDLE)handle, buffer, count, &transferred, NULL)) {
        return -1;
    }
    return transferred;
}

Sb_ssize_t
Sb_FileWrite(void *handle, const void *buffer, Sb_ssize_t count)
{
    DWORD transferred;

    if (!WriteFile((HANDLE)handle, buffer, count, &transferred, NULL)) {
        return -1;
    }
    return transferred;
}

Sb_size_t
Sb_FileTell(void *handle)
{
    return Sb_FileSeek(handle, 0, FILE_CURRENT);
}

Sb_size_t
Sb_FileSeek(void *handle, Sb_ssize_t offset, int whence)
{
    LARGE_INTEGER distance;
    LARGE_INTEGER new_pos;

    distance.QuadPart = offset;
    if (SetFilePointerEx((HANDLE)handle, distance, &new_pos, whence) < 0) {
        return -1;
    }

    return (Sb_size_t)new_pos.QuadPart;
}

void
Sb_FileClose(void *handle)
{
    CloseHandle((HANDLE)handle);
}

void *
Sb_GetStdInHandle(void)
{
    return (void *)GetStdHandle(STD_INPUT_HANDLE);
}

void *
Sb_GetStdOutHandle(void)
{
    return (void *)GetStdHandle(STD_OUTPUT_HANDLE);
}

void *
Sb_GetStdErrHandle(void)
{
    return (void *)GetStdHandle(STD_ERROR_HANDLE);
}
