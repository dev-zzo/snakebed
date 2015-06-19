#include "bstrap.h"

/* Relying on compiler here. */
#include <stdlib.h>

/* Space for converting a 64-bit with radix 2 */
static char xtoa_buffer[68];

char *
Sb_ULtoA(unsigned long x, unsigned radix)
{
    char *q;

    /* NOTE: the implementation is not thread safe, but who cares. */

    q = &xtoa_buffer[sizeof(xtoa_buffer) - 1];
    while (x) {
        unsigned long quot, rem;

        quot = x / radix;
        rem = x % radix;
        x = quot;

        *--q = (char)(rem > 9 ? ('a' + rem - 10) : ('0' + rem));
    }

    return q;
}

char *
Sb_LtoA(long x, int radix)
{
    char *q;

    /* NOTE: the implementation has issues with converting 0x80000000 */
    /* NOTE: the implementation is not thread safe, but who cares. */

    if (x < 0) {
        q = Sb_ULtoA(-x, radix);
        *--q = '-';
        return q;
    }
    else {
        return Sb_ULtoA(x, radix);
    }
}

static long
convert_digit(char ch, unsigned radix)
{
    unsigned value;
    
    value = ch - '0';
    if (value < radix)
        return value;
    value = 10 + (ch | 0x20) - 'a';
    if (value < radix)
        return value;
    return -1;
}

int
Sb_AtoL(const char *str, const char **pend, unsigned radix, long *result)
{
    long value = 0;
    const char *str_start;
    int negative = 0;

    /* Validate inputs */
    if (radix > 0 && (radix > 36 || radix < 2)) {
        return -1;
    }
    if (!str) {
        return -1;
    }

    /* Consume leading whitespace */
    while (Sb_IsWhiteSpace(*str)) {
        ++str;
    }

    /* Note the sign, if any */
    if (*str == '-') {
        negative = 1;
        ++str;
    }
    else if (*str == '+') {
        ++str;
    }

    /* Handle cases where we need to guess the radix */
    if (radix == 0) {
        radix = 10;
        if (str[0] == '0') {
            if (str[1] == 'x' || str[1] == 'X') {
                radix = 16;
                str += 2;
            }
            else {
                radix = 8;
                str += 1;
            }
        }
    }

    /* Convert the digits */
    str_start = str;
    while (*str) {
        long digit = convert_digit(*str, radix);
        if (digit < 0)
            break;
        /* NOTE: Handle overflows? */
        value = value * radix + digit;
        ++str;
    }
    if (str_start == str) {
        return -1;
    }

    if (pend) {
        *pend = str;
    }

    if (result) {
        *result = value;
    }
    return 0;
}

int
Sb_AtoUL(const char *str, const char **pend, unsigned radix, unsigned long *result)
{
    unsigned long value = 0;
    const char *str_start;
    int negative = 0;

    /* Validate inputs */
    if (radix > 0 && (radix > 36 || radix < 2)) {
        return -1;
    }
    if (!str) {
        return -1;
    }

    /* Consume leading whitespace */
    while (Sb_IsWhiteSpace(*str)) {
        ++str;
    }

    /* Handle cases where we need to guess the radix */
    if (radix == 0) {
        radix = 10;
        if (str[0] == '0') {
            if (str[1] == 'x' || str[1] == 'X') {
                radix = 16;
                str += 2;
            }
            else {
                radix = 8;
                str += 1;
            }
        }
    }

    /* Convert the digits */
    str_start = str;
    while (*str) {
        long digit = convert_digit(*str, radix);
        if (digit < 0)
            break;
        /* NOTE: Handle overflows? */
        value = value * radix + (unsigned long)digit;
        ++str;
    }
    if (str_start == str) {
        return -1;
    }

    if (pend) {
        *pend = str;
    }

    if (result) {
        *result = value;
    }
    return 0;
}
