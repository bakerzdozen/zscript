#ifndef SCRIPT_BUILTINS_H
#define SCRIPT_BUILTINS_H

#include "script_stddefs.h"

typedef script_int(*ScriptFunc_t)(Args_t*);
typedef script_int(*ScriptCtrlFunc_t)(Element_t*);

/********************************
 Builtin function prototypes
 ********************************/
script_int script_print(Args_t* args);
script_int script_size(Args_t* args);
script_int script_type(Args_t* args);
script_int script_readline(Args_t* args);

#endif