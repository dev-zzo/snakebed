#ifndef __SNAKEBED_MODULE_STRING_H
#define __SNAKEBED_MODULE_STRING_H
#ifdef __cplusplus
extern "C" {
#endif

/* Implements the `__builtin__` module. */
    
extern SbObject *Sb_ModuleString;

typedef struct {
    char filler;
    char align_flag;
    char sign_flag;
    char use_alt_form;
    char use_precision;
    char conv_type;
    unsigned long min_width;
    unsigned long precision;
} SbString_FormatSpecifier;


int
SbString_ParseFormatSpec(const char *spec, SbString_FormatSpecifier* result);

#ifdef __cplusplus
}
#endif
#endif /* __SNAKEBED_MODULE_STRING_H */
