#ifndef __SNAKEBED_IMPORT_H
#define __SNAKEBED_IMPORT_H
#ifdef __cplusplus
extern "C" {
#endif

/* Creates an empty module with the given name and stores it in the registry.
   The name passed will be used for module lookups.
   Returns: Borrowed reference */
SbObject *
Sb_InitModule(const char *name);

/* Loads a module from the given file.
   The name passed will be used for module lookups.
   NOTE: This executes the module's code object.
   Returns: Borrowed reference */
SbObject *
Sb_LoadModule(const char *name, const char *path);

/* Imports a module with the given name.
   This either results in a cached module or a new one.
   Returns: New reference.
*/
SbObject *
SB_Import(const char *name);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_IMPORT_H
