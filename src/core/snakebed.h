#ifndef __SNAKEBED_H
#define __SNAKEBED_H
#ifdef __cplusplus
extern "C" {
#endif

#include "supports.h"
#include "bstrap.h"

#include "object.h"
#include "object_type.h"
#include "object_tuple.h"
#include "object_str.h"
#include "object_dict.h"
#include "object_list.h"
#include "object_int.h"
#include "object_bool.h"
#include "object_cfunc.h"
#include "object_method.h"
#include "object_code.h"
#include "object_frame.h"

#include "exception.h"

/* Nothing. */
#define STATIC 

extern int
Sb_Initialize(void);


#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_H
