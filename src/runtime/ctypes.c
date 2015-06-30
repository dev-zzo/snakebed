#include "runtime.h"

int Sb_IsDigit(char c)
{
    return c >= '0' && c <= '9';
}

int Sb_IsWhiteSpace(char c)
{
    return c == ' ' || c == '\t';
}
