#ifndef __SNAKEBED_OPCODE_H
#define __SNAKEBED_OPCODE_H
#ifdef __cplusplus
extern "C" {
#endif

/* Instruction opcodes for compiled code */
/* Ref: https://docs.python.org/2/library/dis.html */
typedef enum _SbOpcode {
    /* ??? */
    StopCode                = 0,
    PopTop                  = 1,
    RotTwo                  = 2,
    RotThree                = 3,
    DupTop                  = 4,
    RotFour                 = 5,
    Nop                     = 9,

    UnaryPositive           = 10,
    UnaryNegative           = 11,
    UnaryNot                = 12,
    UnaryConvert            = 13,
    UnaryInvert             = 15,

    BinaryPower             = 19,
    BinaryMultiply          = 20,
    BinaryDivide            = 21,
    BinaryModulo            = 22,
    BinaryAdd               = 23,
    BinarySubtract          = 24,
    BinarySubscript         = 25,
    BinaryFloorDivide       = 26,
    BinaryTrueDivide        = 27,
    InPlaceFloorDivide      = 28,
    InPlaceTrueDivide       = 29,

    Slice                   = 30,
    /* Also uses 31-33 */

    StoreSlice              = 40,
    /* Also uses 41-43 */

    DeleteSlice             = 50,
    /* Also uses 51-53 */

    StoreMap                = 54,
    InPlaceAdd              = 55,
    InPlaceSubtract         = 56,
    InPlaceMultiply         = 57,
    InPlaceDivide           = 58,
    InPlaceModulo           = 59,
    StoreSubscript          = 60,
    DeleteSubscript         = 61,

    BinaryLeftShift         = 62,
    BinaryRightShift        = 63,
    BinaryAnd               = 64,
    BinaryXor               = 65,
    BinaryOr                = 66,
    InPlacePower            = 67,
    GetIter                 = 68,

    PrintExpr               = 70,
    PrintItem               = 71,
    PrintNewline            = 72,
    PrintItemTo             = 73,
    PrintNewlineTo          = 74,

    InPlaceLeftShift        = 75,
    InPlaceRightShift       = 76,
    InPlaceAnd              = 77,
    InPlaceXor              = 78,
    InPlaceOr               = 79,

    BreakLoop               = 80,
    WithCleanup             = 81,
    LoadLocals              = 82,
    ReturnValue             = 83,
    ImportStar              = 84,
    ExecStatement           = 85,
    YieldValue              = 86,
    PopBlock                = 87,
    EndFinally              = 88,
    BuildClass              = 89,

    HaveArgument            = 90,
    /* Opcodes from here have an argument: */

    StoreName               = 90, /* Index in name list */
    DeleteName              = 91, /* Index in name list */
    UnpackSequence          = 92, /* Number of sequence items */
    ForIter                 = 93,
    ListAppend              = 94,
    StoreAttr               = 95, /* Index in name list */
    DeleteAttr              = 96, /* Index in name list */
    StoreGlobal             = 97, /* Index in name list */
    DeleteGlobal            = 98, /* Index in name list */
    DupTopX                 = 99, /* Number of items to duplicate */
    LoadConst               = 100, /* Index in const list */
    LoadName                = 101, /* Index in name list */
    BuildTuple              = 102, /* Number of tuple items */
    BuildList               = 103, /* Number of list items */
    BuildSet                = 104, /* Number of set items */
    BuildMap                = 105, /* Always zero for now */
    LoadAttr                = 106, /* Index in name list */
    CompareOp               = 107, /* Comparison operator */
    ImportName              = 108, /* Index in name list */
    ImportFrom              = 109, /* Index in name list */
    JumpForward             = 110, /* Number of bytes to skip */
    JumpIfFalseOrPop        = 111, /* Target byte offset from beginning of code */
    JumpIfTrueOrPop         = 112,
    JumpAbsolute            = 113,
    PopJumpIfFalse          = 114,
    PopJumpIfTrue           = 115,
    LoadGlobal              = 116, /* Index in name list */
    ContinueLoop            = 119, /* Start of loop (absolute) */
    SetupLoop               = 120, /* Target address (relative) */
    SetupExcept             = 121,
    SetupFinally            = 122,
    LoadFast                = 124, /* Local variable number */
    StoreFast               = 125,
    DeleteFast              = 126,
    RaiseVarArgs            = 130, /* Number of raise arguments (1, 2 or 3) */
    CallFunction            = 131, /* #args + (#kwargs<<8) */
    MakeFunction            = 132, /* #defaults */
    BuildSlice              = 133, /* Number of items */

    MakeClosure             = 134, /* #free vars */
    /* Load free variable from closure */
    LoadClosure             = 135,
    /* Load and dereference from closure cell */
    LoadDeref               = 136,
    /* Store into cell */
    StoreDeref              = 137,

    CallFunctionVar         = 140,
    CallFunctionKw          = 141,
    CallFunctionVarKw       = 142,

    SetupWith               = 143,
    /* Support for opargs more than 16 bits long */
    ExtendedArg             = 145,
    SetAdd                  = 146,
    MapAdd                  = 147,
} SbOpcode;

typedef enum _SbCompareCode {
    PyCmp_LT = 0,
    PyCmp_LE = 1,
    PyCmp_EQ = 2,
    PyCmp_NE = 3,
    PyCmp_GT = 4,
    PyCmp_GE = 5,

    PyCmp_IN,
    PyCmp_NOT_IN,
    PyCmp_IS,
    PyCmp_IS_NOT,
    PyCmp_EXC_MATCH,

    PyCmp_BAD,
} SbCompareCode;

#define SbOpcodeHasArg(op) ((op) >= HaveArgument)

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_OPCODE_H
