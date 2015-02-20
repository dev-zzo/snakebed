#include "snakebed.h"

extern int
_SbType_BuiltinInit();
extern int
_SbType_BuiltinInit2();
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
extern int
_SbCFunction_BuiltinInit();
int
_SbMethod_BuiltinInit();

int
_SbStr_BuiltinInit2();

int
Sb_Initialize()
{
    /* Stage 1: build the most basic types */
    if (_SbType_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbCFunction_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbMethod_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbStr_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbDict_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbTuple_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbNone_BuiltinInit() < 0) {
        return -1;
    }
    /* Stage 2: revisit type objects */
    _SbType_BuiltinInit2();
    _SbStr_BuiltinInit2();
    /* Stage 3: implement all other objects */
    if (_SbObject_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbInt_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbList_BuiltinInit() < 0) {
        return -1;
    }
    if (_SbNotImplemented_BuiltinInit() < 0) {
        return -1;
    }

    return 0;
}
