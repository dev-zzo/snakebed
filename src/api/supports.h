#ifndef __SNAKEBED_SUPOORTS_H
#define __SNAKEBED_SUPOORTS_H

#define ON +
#define OFF -
#define SUPPORTS(x) ((1 x 1) == 2)

/* Define these to either ON or OFF. */

/* Build with object allocation statistics */
#define ALLOC_STATISTICS ON

/* Build with type checks in internal methods */
#define BUILTIN_TYPECHECKS ON

/* Interpreter supports */
#define WITH_STMT OFF

/* Module marshaler supports */
#define UNMARSHAL_LIST OFF
#define UNMARSHAL_DICT OFF

/* Builtin functions supports */
#define BUILTIN_PRINT ON

/* Builtin `str` related supports */
#define STRING_INTERPOLATION ON
#define STR_FORMAT ON

/* Builtin pretty traceback support */
#define TRACEBACKS ON

/* Compiled-in modules support */

/* Around 4k on x86 */
#define MODULE_SOCKET ON

#if SUPPORTS(TRACEBACKS)
#define MODULE_TRACEBACK ON
#endif

#endif // __SNAKEBED_SUPOORTS_H
