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
_SbStr_BuiltinInit2();
extern int
_SbDict_BuiltinInit();
extern int
_SbNone_BuiltinInit();
extern int
_SbNotImplemented_BuiltinInit();
extern int
_SbCFunction_BuiltinInit();
extern int
_SbCFunction_BuiltinInit2();
extern int
_SbPFunction_TypeInit();
extern int
_SbMethod_BuiltinInit();
extern int
_SbErr_BuiltinInit();
extern int
_SbModule_TypeInit();
extern int
_SbFile_TypeInit();
extern int
_SbCode_TypeInit();
extern int
_SbFrame_TypeInit();

typedef int (*typeinitfunc)();

static typeinitfunc stage1_inits[] = {
    _SbType_BuiltinInit,
    _SbCFunction_BuiltinInit,
    _SbStr_BuiltinInit,
    _SbDict_BuiltinInit,
    /* Sentinel */
    NULL
};

static typeinitfunc stage2_inits[] = {
    _SbType_BuiltinInit2,
    _SbStr_BuiltinInit2,
    _SbCFunction_BuiltinInit2,
    /* Sentinel */
    NULL
};

static typeinitfunc stage3_inits[] = {
    _SbErr_BuiltinInit,
    _SbTuple_BuiltinInit,
    _SbList_BuiltinInit,
    _SbInt_BuiltinInit,
    _SbNone_BuiltinInit,
    _SbNotImplemented_BuiltinInit,
    _SbPFunction_TypeInit,
    _SbMethod_BuiltinInit,
    _SbObject_BuiltinInit,
    _SbModule_TypeInit,
    _SbFile_TypeInit,
    _SbCode_TypeInit,
    _SbFrame_TypeInit,
    /* Sentinel */
    NULL
};

static int
do_initfuncs(typeinitfunc *funcs)
{
    for (;;) {
        typeinitfunc f = *funcs;
        if (!f) {
            break;
        }
        if (f() < 0) {
            return -1;
        }
        funcs++;
    }
    return 0;
}

int
Sb_Initialize(void)
{
    /* Stage 1: build the most basic types */
    if (do_initfuncs(stage1_inits) < 0) {
        return -1;
    }
    /* Stage 2: revisit type objects */
    if (do_initfuncs(stage2_inits) < 0) {
        return -1;
    }
    /* Stage 3: implement all other objects */
    if (do_initfuncs(stage3_inits) < 0) {
        return -1;
    }

    return 0;
}
