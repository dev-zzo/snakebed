
#include "platform.h"
#include <malloc.h>

void
SbRT_BZero(void *ptr, Sb_size_t size);

void *
Sb_Malloc(Sb_size_t size)
{
    return malloc(size);
}

void *
Sb_Calloc(Sb_size_t count, Sb_size_t size)
{
    Sb_size_t bytes;
    void *p;

    bytes = count * size;
    if (bytes < count) {
        return NULL;
    }
    p = Sb_Malloc(bytes);
    if (p) {
        SbRT_BZero(p, bytes);
    }
    return p;
}

void *
Sb_Realloc(void *ptr, Sb_size_t new_size)
{
    return realloc(ptr, new_size);
}

void
Sb_Free(void *ptr)
{
    free(ptr);
}
