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

