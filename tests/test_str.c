#include "snakebed.h"

static int test_str_format()
{
    SbObject *r;
    const char *buffer;

    r = SbStr_FromFormat("aaa %% %c %d %i %u %x %p %s",
        'X', -645, 0x80000001, 0x98765432U, 0xabcdef12U, 0xdeadbabe, "testing");
    buffer = SbStr_AsStringUnsafe(r);
    if (Sb_StrCmp(buffer, "aaa % X -645 -2147483647 2557891634 abcdef12 0xdeadbabe testing")) {
        return -1;
    }
    Sb_DECREF(r);

    return 0;
}

int
test_str_main(int which)
{
    switch (which) {
    case 0: return test_str_format();
    default: return 1;
    }
}
