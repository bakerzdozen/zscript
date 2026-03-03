#ifndef ZSCRIPT_ERR_H
#define ZSCRIPT_ERR_H

#include <stdio.h>
#include "script_stddefs.h"

script_int print_script_error(script_int err_type, Script_t* script);
void print_general_error(const char* fmt, ...);

#endif  // ZSCRIPT_ERR_H