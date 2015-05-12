#ifndef __SNAKEBED_ARGS_H
#define __SNAKEBED_ARGS_H
#ifdef __cplusplus
extern "C" {
#endif

/* Parse positinal and keyword arguments, assigning the pointers as appropriate.
   Note: All the returned references are borrowed.
   Returns: 0 if OK, -1 on error.
*/
int
SbArgs_Parse(SbObject *args, SbObject *kwds, Sb_ssize_t count_min, Sb_ssize_t count_max, const char *names[], ...);

/* Unpack positional arguments, assigning pointers as appropriate.
   Note: All the returned references are borrowed.
   Returns: 0 if OK, -1 on error.
*/
int
SbArgs_Unpack(SbObject *args, Sb_ssize_t count_min, Sb_ssize_t count_max, ...);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_ARGS_H
