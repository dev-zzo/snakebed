#ifndef __SNAKEBED_H
#define __SNAKEBED_H
#ifdef __cplusplus
extern "C" {
#endif

#include "supports.h"
#include "runtime.h"

#include "object.h"
#include "object_type.h"
#include "object_tuple.h"
#include "object_str.h"
#include "object_dict.h"
#include "object_list.h"
#include "object_int.h"
#include "object_exc.h"
#include "object_bool.h"
#include "object_code.h"
#include "object_cfunc.h"
#include "object_pfunc.h"
#include "object_method.h"
#include "errors.h"
#include "args.h"
#include "object_frame.h"
#include "object_module.h"
#include "object_file.h"
#include "object_slice.h"
#include "object_iter.h"
#include "proto.h"

#include "interp.h"
#include "import.h"

#include "module/builtin.h"
#include "module/sys.h"
#include "module/_string.h"


/* Nothing. */
#define STATIC 

extern int
Sb_Initialize(void);

extern void
Sb_Finalize(void);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_H
