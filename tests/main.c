
#include "snakebed.h"

int
test_lists_main(int which);

int main()
{
    int which;

    Sb_Initialize();

    for (which = 0; ; ++which) {
        int result;

        result = test_lists_main(which);
        if (result != 0) {
            break;
        }
    }

    return 0;
}
