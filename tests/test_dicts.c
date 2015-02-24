#include "snakebed.h"

/* Test: Verify *_New() works as designed. */
static int
test_dict_new(void)
{
    SbObject *dict;

    dict = SbDict_New();
    if (!dict) {
        return -1;
    }
    if (SbDict_GetSize(dict) != 0) {
        return -2;
    }
    Sb_DECREF(dict);

    return 0;
}

static int
test_dict_getsetstring(void)
{
    SbObject *dict;
    SbObject *i1, *i2;

    dict = SbDict_New();
    if (!dict) {
        return -1;
    }

    i1 = SbInt_FromLong(1);
    i2 = SbInt_FromLong(2);

    /* Setting values */
    if (SbDict_SetItemString(dict, "a", i1) < 0) {
        return -2;
    }
    if (SbDict_SetItemString(dict, "b", i2) < 0) {
        return -3;
    }
    if (SbDict_GetSizeUnsafe(dict) != 2) {
        return -4;
    }
    if (SbDict_GetItemString(dict, "a") != i1) {
        return -5;
    }
    if (SbDict_GetItemString(dict, "b") != i2) {
        return -6;
    }

    /* Overwriting values */
    if (SbDict_SetItemString(dict, "a", i2) < 0) {
        return -11;
    }
    if (SbDict_GetItemString(dict, "a") != i2) {
        return -12;
    }

    /* TBD: Setting values with keys having the same hash */

    Sb_DECREF(dict);
    Sb_DECREF(i1);
    Sb_DECREF(i2);
    return 0;
}

int
test_dicts_main(int which)
{
    switch (which) {
    case 0: return test_dict_new();
    case 1: return test_dict_getsetstring();
    default:
        return 1;
    }
}
