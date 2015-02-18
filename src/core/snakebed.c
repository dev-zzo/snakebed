#include "snakebed.h"

extern int
_SbType_BuiltinInit();
extern int
_SbObject_BuiltinInit();
extern int
_SbInt_BuiltinInit();
extern int
_SbTuple_BuiltinInit();
extern int
_SbList_BuiltinInit();
extern int
_SbStr_BuiltinInit();
extern int
_SbDict_BuiltinInit();
extern int
_SbNone_BuiltinInit();
extern int
_SbNotImplemented_BuiltinInit();

int
Sb_Initialize()
{
    /* TODO: Make this a pointer array instead */
    if (_SbType_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbObject_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbInt_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbTuple_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbList_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbStr_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbDict_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbNone_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbNotImplemented_BuiltinInit() < 0) {
        return -1;
    }

    return 0;
}
