#include "snakebed.h"

int main(int argc, const char *argv[])
{
    SbObject *main_module;
    SbObject *exc;
    int rv = 0;

    if (Sb_Initialize() < 0) {
        return 2;
    }

    if (argc < 2) {
        return 3;
    }

    main_module = Sb_LoadModule("__main__", argv[1]);
    SbErr_Fetch(&exc);
    Sb_XDECREF(main_module);
    if (exc) {
        if (SbErr_ExceptionMatches(exc, (SbObject *)SbErr_SystemExit)) {
            SbErr_Clear();
            rv = 0;
        }
        else {
            if (SbErr_ExceptionMatches(exc, (SbObject *)SbErr_MemoryError)) {
                SbFile_WriteString(SbSys_StdErr, "OOM DEATH!\r\n");
            }
            else {
                SbObject *error_str;

                error_str = SbObject_Str(SbErr_GetValue(exc));
                SbFile_WriteString(SbSys_StdErr, "Uncaught exception:\r\n");
                SbFile_WriteString(SbSys_StdErr, SbStr_AsStringUnsafe(error_str));
                SbFile_Write(SbSys_StdErr, "\r\n", 2);
            }
            rv = 1;
        }
    }

    _Sb_UnloadModule("__main__");
    Sb_Finalize();

    return rv;
}
