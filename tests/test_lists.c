#include "snakebed.h"
#include "object_list.h"
#include "object_int.h"

/* Test: Verify *_New() works as designed. */
static int
test_list_new(void)
{
    SbObject *list;

    list = SbList_New(5);
    if (!list) {
        return -1;
    }
    if (SbList_GetSize(list) != 5) {
        return -2;
    }
    Sb_DECREF(list);

    list = SbList_New(0);
    if (!list) {
        return -21;
    }
    Sb_DECREF(list);

    list = SbList_New(-1);
    if (list) {
        Sb_DECREF(list);
        return -31;
    }
    /* No DECREF: list is NULL. */

    return 0;
}

/* Test: Verify SbList_{Get|Set}() works as designed. */
static int
test_list_dtor(void)
{
    SbObject *i1;
    SbObject *list;

    i1 = SbInt_FromLong(1);
    list = SbList_New(1);
    if (!list) {
        return -1;
    }
    SbList_SetItemUnsafe(list, 0, i1);
    Sb_INCREF(i1);
    if (Sb_REFCNT(i1) != 2) {
        return -2;
    }
    Sb_DECREF(list);
    if (Sb_REFCNT(i1) != 1) {
        return -3;
    }

    return 0;
}

/* Test: Verify SbList_{Get|Set}() works as designed. */
static int
test_list_getset(void)
{
    SbObject *i1, *i2, *i3, *i4;
    SbObject *list;

    i1 = SbInt_FromLong(1);
    i2 = SbInt_FromLong(2);
    i3 = SbInt_FromLong(3);
    i4 = SbInt_FromLong(4);

    list = SbList_Pack(3, i1, i2, i3);
    if (!list) {
        return -1;
    }
    if (Sb_REFCNT(i1) != 1 || Sb_REFCNT(i2) != 1 || Sb_REFCNT(i3) != 1) {
        return -2;
    }
    if (SbList_GetItem(list, 0) != i1) {
        return -3;
    }
    if (SbList_GetItem(list, 1) != i2) {
        return -4;
    }

    Sb_INCREF(i3);
    if (SbList_SetItem(list, 2, i4) < 0) {
        return -11;
    }
    if (Sb_REFCNT(i3) != 1) {
        return -12;
    }
    if (Sb_REFCNT(i4) != 1) {
        return -13;
    }

    Sb_DECREF(list);
    Sb_DECREF(i3);

    return 0;
}

static int
test_list_check_exact(void)
{
    SbObject *list;

    list = SbList_New(5);
    return 0;
}

int
test_lists_main(int which)
{
    switch (which) {
    case 0: return test_list_new();
    case 1: return test_list_dtor();
    case 2: return test_list_getset();
    default:
        return 1;
    }



}
