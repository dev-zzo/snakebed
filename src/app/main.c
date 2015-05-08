#include "snakebed.h"

int main(int argc, const char *argv[])
{
    SbObject *main_module;
    int rv = 0;

    if (Sb_Initialize() < 0) {
        return 2;
    }

    if (argc < 2) {
        return 3;
    }

    main_module = Sb_LoadModule("__main__", argv[1]);
    Sb_DECREF(main_module);
    if (SbErr_Occurred()) {
        if (SbErr_ExceptionMatches(SbErr_Occurred(), (SbObject *)SbErr_SystemExit)) {
            SbErr_Clear();
        }
        else {
            /* __asm int 3; */
            rv = 1;
        }
    }

    return rv;
}
