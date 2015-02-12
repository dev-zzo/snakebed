
#include "platform.h"
#include <malloc.h>

void *
Sb_Malloc(Sb_size_t size)
{
    return malloc(size);
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
