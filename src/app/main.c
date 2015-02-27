#include "snakebed.h"

int main(int argc, const char *argv[])
{
    Sb_Initialize();

    if (argc < 2) {
        return 0;
    }

    Sb_LoadModule("__main__", argv[1]);

    return 0;
}
