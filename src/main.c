#include "snakebed.h"

int main(int argc, const char *argv[])
{
    SbObject *main_module;
    SbTypeObject *exc_type;
    SbObject *exc_value;
    SbObject *exc_tb;
    int rv = 0;

    if (Sb_Initialize() < 0) {
        return 2;
    }

    if (argc < 2) {
        return 3;
    }

    main_module = Sb_LoadModule("__main__", argv[1]);
    SbErr_Fetch(&exc_type, &exc_value, &exc_tb);
    Sb_XDECREF(main_module);
    if (exc_type) {
        if (SbExc_ExceptionTypeMatches(exc_type, (SbObject *)SbExc_SystemExit)) {
            SbErr_Clear();
            rv = 0;
        }
        else {
            if (SbExc_ExceptionTypeMatches(exc_type, (SbObject *)SbExc_MemoryError)) {
                SbFile_WriteString(SbSys_StdErr, "OOM DEATH!\r\n");
            }
            else {
                SbTraceBack_PrintException(exc_type, exc_value, exc_tb, 10, SbSys_StdErr);
            }
            rv = 1;
        }
        Sb_XDECREF(exc_tb);
        Sb_XDECREF(exc_value);
        Sb_XDECREF(exc_type);
    }

    _Sb_UnloadModule("__main__");
    Sb_Finalize();

    return rv;
}
