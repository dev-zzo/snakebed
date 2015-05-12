#include "../bstrap.h"
#include <windows.h>

static char _strerror_buffer[512];

const char *
Sb_StrError(OSError_t error_code)
{
    DWORD rv;

    rv = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        (DWORD)error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        _strerror_buffer,
        sizeof(_strerror_buffer),
        NULL);
    if (!rv) {
        return NULL;
    }
    if (rv >= 2 && _strerror_buffer[rv - 2] == '\r') {
        _strerror_buffer[rv - 2] = '\0';
    }
    return _strerror_buffer;
}

