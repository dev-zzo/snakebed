#include "snakebed.h"
#include <stdio.h>

int
test_lists_main(int which);
int
test_dicts_main(int which);

typedef int (*testsuiteproc)(int which);

int do_tests(testsuiteproc suite)
{
    int which;

    printf("[");
    for (which = 0; ; ++which) {
        int result;

        result = suite(which);
        if (result == 1) {
            break;
        }
        printf(result < 0 ? "!" : ".");
    }
    printf("]\n");
    return 0;
}

int main()
{
    Sb_Initialize();

    do_tests(test_lists_main);
    do_tests(test_dicts_main);

    return 0;
}
