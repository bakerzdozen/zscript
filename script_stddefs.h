#ifndef SCRIPT_STDDEFS_H
#define SCRIPT_STDDEFS_H

#include <stdint.h>

#define FUNC_INVALID            0
#define FUNC_BUILTIN            1
#define FUNC_CUSTOM             2

#define SCRIPT_TRUE             1
#define SCRIPT_FALSE            0

#define DEFAULT_BUFF_SIZE       16

typedef struct _script Script_t;

typedef enum _type {
    SCRIPT_TYPE_INVALID,
    /* Start of types which support mathematical operators */
    SCRIPT_TYPE_INT,
    SCRIPT_TYPE_FLOAT,
    SCRIPT_TYPE_BOOL,
    /* End of types which support mathematical operators */
    SCRIPT_TYPE_TYPE,
    SCRIPT_TYPE_STRING,
    SCRIPT_TYPE_LIST,
    SCRIPT_TYPE_FUNC,
    SCRIPT_TYPE_CONST = 1 << 3,
    ADD = 1 << 4,
    SUBTRACT = 2 << 4,
    MULTIPLY = 3 << 4,
    DIVIDE = 4 << 4,
    DIFFER = 5 << 4,
    EQUALILTY = 6 << 4,
    GREATER_THAN = 7 << 4,
    LESS_THAN = 8 << 4,
    GT_EQUALS = 9 << 4,
    LT_EQUALS = 10 << 4,
    AND = 11 << 4,
    OR = 12 << 4,
} VarType_t;

typedef enum _keyword {
    SPACE,
    NEW_LINE,
    END_EXPRESSION,
    FWD_SLASH,
    IF,
    ELSE,
    WHILE,
    FOR,
    BREAK,
    RETURN,
    EQUALS,
    EXCLAMATION,
    LOGIC_GT,
    LOGIC_GTE,
    LOGIC_LT,
    LOGIC_LTE,
    LOGIC_AND,
    LOGIC_OR,
    LOGIC_NE,
    ADDITION,
    SUBTRACTION,
    MULTIPLICATION,
    BRACKET_STD_OPEN,
    BRACKET_STD_CLOSE,
    BRACKET_SQUARE_OPEN,
    BRACKET_SQUARE_CLOSE,
    BRACKET_CURL_OPEN,
    BRACKET_CURL_CLOSE,
    UNKOWN_STR,
    LITERAL_INT,
    LITERAL_BOOL,
    LITERAL_FLOAT,
    LITERAL_STR,
    LITERAL_CHAR,
    LITERAL_LIST,
    VARIABLE_UNDEFINED,
    VARIABLE_INT,
    VARIABLE_BOOL,
    VARIABLE_FLOAT,
    VARIABLE_STR,
    VARIABLE_CHAR,
    VARIABLE_LIST,
    COMMA,
    SIZE_OVERFLOW,
    LITERAL_STR_BUFF_FULL,
    LITERAL_STR_END,
    NUM_KEYWORDS,
    FILE_END,
    PARSE_ERR
} Keyword_t;    // TODO: Ensure smallest int possible

typedef int script_int;

typedef int ElementID_t;

typedef enum _script_err {
    SCRIPT_ERR_START = -128,
    SCRIPT_ERR_NO_VAR,
    SCRIPT_ERR_FORMAT,
    SCRIPT_ERR_INCOMPATIBLE_OP,
    SCRIPT_ERR_PARSE,
    SCRIPT_ERR_LIST,
    SCRIPT_ERR_TOO_MANY_ELEMENTS,
    SCRIPT_ERR_NAME_TOO_LONG,
    SCRIPT_ERR_NO_FILE,
    SCRIPT_OK = 1,
} ScriptError_t;

typedef struct _Element {
    char* name;
    script_int id;
    uint16_t size;
    union {
        void* ptr;
        script_int num_i;
        float num_f;
    } data;
} Element_t;

typedef struct _args {
    Element_t* input_args;
    Element_t* output_args;
} Args_t;

#endif