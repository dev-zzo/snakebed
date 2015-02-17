#include "snakebed.h"

int
_Sb_CreateBuiltins();

int
Sb_Initialize()
{
    if (_Sb_CreateBuiltins() < 0) {
        return -1;
    }

    return 0;
}
