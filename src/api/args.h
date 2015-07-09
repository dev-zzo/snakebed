#ifndef __SNAKEBED_ARGS_H
#define __SNAKEBED_ARGS_H
#ifdef __cplusplus
extern "C" {
#endif

/* Parse the arguments specifier and populate (and possibly convert) the args.

Conversion specifiers are as follows.

Not type checked:
* O: Raw SbObject pointer (borrowed ref)

Type checked but not converted (borrowed ref):
* T: tuple object
* L: list object
* D: dict object
* S: str object

Type checked and converted:
* i: int, converted to SbInt_Native_t
* s: str, converted to const char *
* c: str of length 1, converted to char

   Returns: 0 if OK, -1 otherwise. */
*/

int
SbArgs_Parse(const char *spec, SbObject *args, SbObject *kwds, ...);

/* Ensure the function was not passed any args.
   Returns: 0 if OK, -1 otherwise. */
*/
int
SbArgs_NoArgs(SbObject *args, SbObject *kwds);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_ARGS_H
