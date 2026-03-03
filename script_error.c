#include "script_error.h"
#include "script_parser.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static void display_script_error(script_int ln, script_int ret)
{
    switch (ret) {
        case SCRIPT_ERR_NO_VAR:
            printf("Tried to use a variable which was not declared\n");
            break;
        case SCRIPT_ERR_FORMAT:
            printf("Formatting error\n");
            break;
        case SCRIPT_ERR_INCOMPATIBLE_OP:
            printf("Incompatible operand for the given type/s\n");
            break;
        case SCRIPT_ERR_PARSE:
            printf("Parsing error\n");
            break;
        case SCRIPT_ERR_LIST:
            printf("List error\n");
            break;
        case SCRIPT_ERR_TOO_MANY_ELEMENTS:
            printf("Too many elements in function call\n");
            break;
        case SCRIPT_ERR_NAME_TOO_LONG:
            printf("Element name too long. Please keep it less than %u characters\n", DEFAULT_BUFF_SIZE);
            break;
        case SCRIPT_ERR_NO_FILE:
            printf("No file found\n");
            break;
        default:
            break;
    }
}

script_int print_script_error(script_int err_type, Script_t* script)
{
    static bool displayed = false;

    if (displayed || (err_type >= 0)) {
        return err_type;
    }

    int line_no = get_line_number(script);
        
    if (line_no <= 0) {
        return err_type;
    }

    printf("\x1b[1;31m");
    printf("Error at line %u\n", line_no);
    printf("\x1b[0m");
    printf("\x1b[31m");

    int line_len = print_line(script, line_no);
    
    print_n_chars('^', line_len);

    putchar('\n');
    
    display_script_error(line_no, err_type);
    
    printf("\x1b[0m");

    displayed = true;

    return err_type;
}

void print_general_error(const char* fmt, ...)
{
    if ((fmt == NULL) || (strlen(fmt) == 0)) {
        printf("Error");
        return;
    }

    printf("Error - ");

    va_list args;
    va_start(args, fmt);

    vprintf(fmt, args);    
 
    va_end(args);
}