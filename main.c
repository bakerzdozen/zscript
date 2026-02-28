// BUILTIN headers
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <time.h>

// ZSCRIPT headers
#include "script_error.h"
#include "script_parser.h"
#include "script_builtins.h"
#include "script_stddefs.h"

#define PRINTOUT

script_int run(Script_t* file, script_int in_func);
script_int script_if(Element_t* main_element);
script_int script_while(Element_t* main_element);
script_int get_element(Element_t* main_element, Element_t* out, char in_brackets);
script_int function_statement(Element_t* main_element, Element_t** out);
script_int script_return(Element_t* main_element);

#ifdef PRINTOUT
#define SCRIPT_RETURN(x, script)    return print_error(x, script);
#else
#define SCRIPT_RETURN(x, _)         return x;
#endif

Element_t global_variable_dict[DEFAULT_BUFF_SIZE] = {
    {
        .name = "true",
        .data.num_i = SCRIPT_TRUE,
        .id = SCRIPT_TYPE_BOOL | SCRIPT_TYPE_CONST,
        .size = 1
    },
    {
        .name = "false",
        .data.num_i = SCRIPT_FALSE,
        .id = SCRIPT_TYPE_BOOL | SCRIPT_TYPE_CONST,
        .size = 1
    }
};

script_int local_variable_size = 0;
Element_t* local_variable_dict = NULL;
script_int return_count = 0;

Element_t function_dict[DEFAULT_BUFF_SIZE] = {
    {
        .name = "output",
        .data.ptr = script_print,
        .size = (1 & 0xF) | (1 << 4),   // (Number of input args & 0xF) | (Number of output args << 4)
        .id = FUNC_BUILTIN              // builtin funcs use .data for pointer. Custom funcs use data for position in file
    },
    {
        .name = "size",
        .data.ptr = script_size,
        .size = (1 & 0xF) | (1 << 4),
        .id = FUNC_BUILTIN
    },
    {
        .name = "type",
        .data.ptr = script_type,
        .size = (1 & 0xF) | (1 << 4),
        .id = FUNC_BUILTIN
    },
    {
        .name = "input",
        .data.ptr = script_readline,
        .size = (0 & 0xF) | (1 << 4),
        .id = FUNC_BUILTIN
    },
};

Element_t ctrl_flow_dict[DEFAULT_BUFF_SIZE] = {
    {
        .name = "if",
        .data.ptr = script_if,
        .size = 1,
        .id = LITERAL_BOOL
    },
    {
        .name = "return",
        .data.ptr = script_return,
        .size = 1,
        .id = INVALID_KEYWORD
    },
    {
        .name = "while",
        .data.ptr = script_while,
        .size = 1,
        .id = LITERAL_BOOL
    }
};


Element_t* get_key(const char* key, Element_t* list) 
{
    // TODO: Make hash list
    for (script_int i = 0; i < DEFAULT_BUFF_SIZE; i++) {
        if (list[i].name == NULL) {
            continue;
        }
        if (strncmp(list[i].name, key, DEFAULT_BUFF_SIZE) == 0) {
            return &list[i];
        }
    }
    return NULL;
}

void put_function(Element_t* function) {
    static int i = 4;
    function_dict[i].data = function->data;
    function_dict[i].id = function->id;
    function_dict[i].size = function->size;
    function_dict[i].name = function->name;
}

void put_element(Element_t* element) {
    static int i = 2;
    global_variable_dict[i].data = element->data;
    global_variable_dict[i].id = element->id;
    global_variable_dict[i].size = element->size;
    global_variable_dict[i++].name = element->name;
}

Element_t* get_builtin(const char* key) {
    // TODO: Make hash list
    for (script_int i = 0; i < 10; i++) {
        if (ctrl_flow_dict[i].name == NULL) {
            continue;
        }
        if (strncmp(ctrl_flow_dict[i].name, key, DEFAULT_BUFF_SIZE) == 0) {
            return &function_dict[i];
        }
    }
    return NULL;
}

script_int script_str_index(const char* str, char to_find)
{
    script_int i = 0;
    char c = str[0];

    while (c != to_find) {
        c = str[i++];
        if (c == '\0') {
            return -1;
        }
    }
    
    return i;
}

script_int parse_element(Element_t* element)
{
    char* end = NULL;
    if (element->id < 0x7) {
        // Already parsed
        return SCRIPT_OK;
    }
    switch (element->id) {
        case BRACKET_SQUARE_OPEN:
            // Must be from a list
            element->id = SCRIPT_TYPE_LIST;
            return SCRIPT_OK;
        case LITERAL_INT:
            element->data.num_i = strtol(element->name, &end, 10);
            element->id = SCRIPT_TYPE_INT;
            if (end == element->name) {
                // We have not parsed any chars
                return SCRIPT_ERR_PARSE;
            }
            return SCRIPT_OK;
        case LITERAL_STR:
            // Might not need?  ==> Special case
            break;
        case LITERAL_FLOAT:
            element->data.num_f = strtof(element->name, &end);
            element->id = SCRIPT_TYPE_FLOAT;
            if (end == element->name) {
                // We have not parsed any chars
                return SCRIPT_ERR_PARSE;
            }
            return SCRIPT_OK;
        case LITERAL_BOOL:
            // element->data.num_i
            // TODO: I don't think this is ever a thing - a bool is just the global variable "true"/"false"
            break;
        case UNKOWN_STR:
            // Must be a variable or function ptr
            Element_t* search_element = get_key(element->name, global_variable_dict);
            if (search_element == NULL) {
                // Variable
                search_element = get_key(element->name, function_dict);
            }
            if ((search_element == NULL) && (local_variable_dict != NULL)) {
                search_element = get_key(element->name, local_variable_dict);
            }
            if (search_element != NULL) {
                strncpy(element->name, search_element->name, DEFAULT_BUFF_SIZE);
                element->data = search_element->data;
                element->size = search_element->size;
                element->id = search_element->id & ~(SCRIPT_TYPE_CONST);
                return SCRIPT_OK;
            }
            break;
        default:
            return SCRIPT_ERR_PARSE;
    }
    return SCRIPT_ERR_PARSE;
}

static VarType_t get_operator(script_int num)
{
    switch (num) {
        case ADDITION:
            return ADD;
        case SUBTRACTION:
            return SUBTRACT;
        case MULTIPLICATION:
            return MULTIPLY;
        case FWD_SLASH:
            return DIVIDE;
        case EQUALS:
            return EQUALILTY;
        case LOGIC_NE:
            return DIFFER;
        case LOGIC_LT:
            return LESS_THAN;
        case LOGIC_LTE:
            return LT_EQUALS;
        case LOGIC_GT:
            return GREATER_THAN;
        case LOGIC_GTE:
            return GT_EQUALS;
        default:
            return SCRIPT_TYPE_INVALID;
    }
}

Element_t* add_to_string(Element_t* string_element, Element_t* element_2)
{
    char buff[DEFAULT_BUFF_SIZE + 1];
    script_int buff_size = 0;

    // Checks
    if ((string_element->id & 0x7) != SCRIPT_TYPE_STRING) {
        return NULL;
    }

    // Step 1: Create return element as copy of string element
    Element_t* res = calloc(1, sizeof(Element_t));
    res->id = SCRIPT_TYPE_STRING;
    res->size = string_element->size + 1;   // Add one to account for null terminator

    // Step 2: Add to end of string
    switch(element_2->id & 0x7) {
        case SCRIPT_TYPE_BOOL:
            if (element_2->data.num_i) {
                // True
                res->size += 4;
                res->data.ptr = calloc(res->size, sizeof(char));
                snprintf(res->data.ptr, res->size, "%s%s", (char*)string_element->data.ptr, "True");
            } else {
                // False
                res->size += 5;
                res->data.ptr = calloc(res->size, sizeof(char));
                snprintf(res->data.ptr, res->size, "%s%s", (char*)string_element->data.ptr, "False");
            }
            break;
        case SCRIPT_TYPE_INT:
            buff_size = snprintf(buff, sizeof(buff), "%i", element_2->data.num_i);
            if (buff_size > sizeof(buff)) {
                buff_size = sizeof(buff);
            }
            res->size += buff_size;
            res->data.ptr = calloc(res->size, sizeof(char));
            snprintf((char*)res->data.ptr, res->size, "%s%s", (char*)string_element->data.ptr, buff);
            break;
        case SCRIPT_TYPE_FLOAT:
            buff_size = snprintf(buff, sizeof(buff), "%f", element_2->data.num_f);
            if (buff_size > sizeof(buff)) {
                buff_size = sizeof(buff);
            }
            res->size += buff_size;
            res->data.ptr = calloc(res->size, sizeof(char));
            snprintf((char*)res->data.ptr, res->size, "%s%s", (char*)string_element->data.ptr, buff);
            break;
        case SCRIPT_TYPE_STRING:
            res->size += element_2->size;
            res->data.ptr = calloc(res->size, sizeof(char));
            snprintf((char*)res->data.ptr, res->size, "%s%s", (char*)string_element->data.ptr, (char*)element_2->data.ptr);
            break;
        case SCRIPT_TYPE_LIST:
            // TODO: Loop through list doing this function
            return NULL; 
    }
    res->size -= 1; // Remove null terminator from size count
    return res;
}

Element_t* add_elements(Element_t* element_1, Element_t* element_2)
{
    if ((element_1 == NULL) || (element_2 == NULL)) {
        return NULL;
    }
    
    VarType_t element_1_id = (element_1->id & 0x7);
    VarType_t element_2_id = (element_2->id & 0x7);

    if ((element_1_id == SCRIPT_TYPE_INVALID) || (element_2_id == SCRIPT_TYPE_INVALID)) {
        return NULL;
    }

    if (element_1_id == SCRIPT_TYPE_STRING) {
        return add_to_string(element_1, element_2);
    }

    if ((element_1_id >= SCRIPT_TYPE_BOOL) || (element_2_id >= SCRIPT_TYPE_BOOL)) {
        return NULL;
    }

    Element_t* res = calloc(1, sizeof(Element_t));

    switch (element_1_id)
    {
    case SCRIPT_TYPE_INT:
        if (element_2_id == SCRIPT_TYPE_FLOAT) {
            res->data.num_f = (float)element_1->data.num_i + element_2->data.num_f;
            res->id = SCRIPT_TYPE_FLOAT;
        } else {
            res->data.num_i = element_1->data.num_i + element_2->data.num_i;
            res->id = SCRIPT_TYPE_INT;
        }
        res->size = 1;
        res->id = element_2_id & 0x7;
        break;
    case SCRIPT_TYPE_FLOAT:
        if (element_2_id == SCRIPT_TYPE_FLOAT) {
            res->data.num_f = element_1->data.num_f + element_2->data.num_f;
        } else {
            res->data.num_f = element_1->data.num_f + (float)element_2->data.num_i;
        }
        res->size = 1;
        res->id = SCRIPT_TYPE_FLOAT;
        break;
    default:
        free(res);
        return NULL;
    }
    return res;
}

Element_t* sub_elements(Element_t* element_1, Element_t* element_2)
{
    if ((element_1 == NULL) || (element_2 == NULL)) {
        return NULL;
    }
    
    VarType_t element_1_id = (element_1->id & 0x7);
    VarType_t element_2_id = (element_2->id & 0x7);

    if ((element_1_id == SCRIPT_TYPE_INVALID) || (element_2_id == SCRIPT_TYPE_INVALID)) {
        return NULL;
    }

    if ((element_1_id >= SCRIPT_TYPE_BOOL) || (element_2_id >= SCRIPT_TYPE_BOOL)) {
        return NULL;
    }

    Element_t* res = calloc(1, sizeof(Element_t));

    switch (element_1_id)
    {
    case SCRIPT_TYPE_INT:
        if (element_2_id == SCRIPT_TYPE_FLOAT) {
            res->data.num_f = (float)element_1->data.num_i - element_2->data.num_f;
            res->id = SCRIPT_TYPE_FLOAT;
        } else {
            res->data.num_i = element_1->data.num_i - element_2->data.num_i;
            res->id = SCRIPT_TYPE_INT;
        }
        res->size = 1;
        res->id = element_2_id;
        break;
    case SCRIPT_TYPE_FLOAT:
        if (element_2_id == SCRIPT_TYPE_FLOAT) {
            res->data.num_f = element_1->data.num_f - element_2->data.num_f;
        } else {
            res->data.num_f = element_1->data.num_f - (float)element_2->data.num_i;
        }
        res->size = 1;
        res->id = SCRIPT_TYPE_FLOAT;
        break;
    default:
        free(res);
        return NULL;
    }
    return res;
}

Element_t* mul_elements(Element_t* element_1, Element_t* element_2)
{
    if ((element_1 == NULL) || (element_2 == NULL)) {
        return NULL;
    }
    
    VarType_t element_1_id = (element_1->id & 0x7);
    VarType_t element_2_id = (element_2->id & 0x7);

    if ((element_1_id == SCRIPT_TYPE_INVALID) || (element_2_id == SCRIPT_TYPE_INVALID)) {
        return NULL;
    }

    if ((element_1_id >= SCRIPT_TYPE_STRING) || (element_2_id >= SCRIPT_TYPE_STRING)) {
        return NULL;
    }

    Element_t* res = calloc(1, sizeof(Element_t));

    switch (element_1_id)
    {
        // TODO: Fix issues with booleans -> should return a boolean
    case SCRIPT_TYPE_BOOL:
    case SCRIPT_TYPE_INT:
        if (element_2_id == SCRIPT_TYPE_FLOAT) {
            res->data.num_f = (float)element_1->data.num_i * element_2->data.num_f;
            res->id = SCRIPT_TYPE_FLOAT;
        } else {
            res->data.num_i = element_1->data.num_i * element_2->data.num_i;
            res->id = SCRIPT_TYPE_INT;
        }
        res->size = 1;
        break;
    case SCRIPT_TYPE_FLOAT:
        if (element_2_id == SCRIPT_TYPE_FLOAT) {
            res->data.num_f = element_1->data.num_f * element_2->data.num_f;
        } else {
            res->data.num_f = element_1->data.num_f * (float)element_2->data.num_i;
        }
        res->size = 1;
        res->id = SCRIPT_TYPE_FLOAT;
        break;
    default:
        free(res);
        return NULL;
    }
    return res;
}

Element_t* div_elements(Element_t* element_1, Element_t* element_2)
{
    if ((element_1 == NULL) || (element_2 == NULL)) {
        return NULL;
    }
    
    VarType_t element_1_id = (element_1->id & 0x7);
    VarType_t element_2_id = (element_2->id & 0x7);

    if ((element_1_id == SCRIPT_TYPE_INVALID) || (element_2_id == SCRIPT_TYPE_INVALID)) {
        return NULL;
    }

    if ((element_1_id >= SCRIPT_TYPE_STRING) || (element_2_id >= SCRIPT_TYPE_STRING)) {
        return NULL;
    }

    Element_t* res = calloc(1, sizeof(Element_t));

    switch (element_1_id)
    {
    case SCRIPT_TYPE_INT:
        if (element_2_id == SCRIPT_TYPE_FLOAT) {
            res->data.num_f = (float)element_1->data.num_i / element_2->data.num_f;
        } else {
            res->data.num_f = (float)element_1->data.num_i / (float)element_2->data.num_i;
        }
        res->size = 1;
        // if (res->data.num_f - (float)((script_int)(res->data.num_f + 0.0001)) < ) {

        // } // TODO: Convert to script_int if possible
        res->id = SCRIPT_TYPE_FLOAT;
        break;
    case SCRIPT_TYPE_FLOAT:
        if (element_2_id == SCRIPT_TYPE_FLOAT) {
            res->data.num_f = element_1->data.num_f / element_2->data.num_f;
        } else {
            res->data.num_f = element_1->data.num_f / (float)element_2->data.num_i;
        }
        res->size = 1;
        res->id = SCRIPT_TYPE_FLOAT;
        break;
    default:
        free(res);
        return NULL;
    }
    return res;
}

Element_t* cmp_elements(Element_t* element_1, Element_t* element_2) 
{
    if ((element_1 == NULL) || (element_2 == NULL)) {
        return NULL;
    }
    
    VarType_t element_1_id = (element_1->id & 0x7);
    VarType_t element_2_id = (element_2->id & 0x7);

    if ((element_1_id == SCRIPT_TYPE_INVALID) || (element_2_id == SCRIPT_TYPE_INVALID)) {
        return NULL;
    }

    Element_t* res = calloc(1, sizeof(Element_t));
    res->id = SCRIPT_TYPE_BOOL;

    if ((element_1_id == SCRIPT_TYPE_STRING) && (element_2_id == SCRIPT_TYPE_STRING)) {
        res->size = element_1->size;
        res->data.num_i = element_1->size == element_2->size;
        if (res->data.num_i == SCRIPT_FALSE) {
            return res;
        }
        res->data.num_i &= strncmp(element_1->data.ptr, element_2->data.ptr, element_1->size) == 0;
        return res;
    } else if ((element_1_id >= SCRIPT_TYPE_STRING) || (element_2_id >= SCRIPT_TYPE_STRING)) {
        free(res);
        return NULL;
    }

    switch (element_1_id) {
        case SCRIPT_TYPE_TYPE:
        case SCRIPT_TYPE_INT:
            if (element_2_id == SCRIPT_TYPE_FLOAT) {
                res->data.num_i = element_1->data.num_i == element_2->data.num_f;
            } else {
                res->data.num_i = element_1->data.num_i == element_2->data.num_i;
            }
            break;
        case SCRIPT_TYPE_FLOAT:
            if (element_2_id == SCRIPT_TYPE_FLOAT) {
                res->data.num_i = element_1->data.num_f == element_2->data.num_f;
            } else {
                res->data.num_i = element_1->data.num_f == element_2->data.num_i;
            }
            break;
        case SCRIPT_TYPE_BOOL:
            if (element_2_id == SCRIPT_TYPE_FLOAT) {
                res->data.num_i = element_1->data.num_i == element_2->data.num_f;
            } else {
                res->data.num_i = element_1->data.num_i == element_2->data.num_i;
            }
            break;
        default:
            free(res);
            return NULL;
    }
    res->size = 1;
    return res;
}

Element_t* gt_elements(Element_t* element_1, Element_t* element_2) 
{
    if ((element_1 == NULL) || (element_2 == NULL)) {
        return NULL;
    }
    
    VarType_t element_1_id = (element_1->id & 0x7);
    VarType_t element_2_id = (element_2->id & 0x7);

    if ((element_1_id == SCRIPT_TYPE_INVALID) || (element_2_id == SCRIPT_TYPE_INVALID)) {
        return NULL;
    }
    
    if ((element_1_id >= SCRIPT_TYPE_BOOL) || (element_2_id >= SCRIPT_TYPE_BOOL)) {
        return NULL;
    }
    
    Element_t* res = calloc(1, sizeof(Element_t));
    res->id = SCRIPT_TYPE_BOOL;

    switch (element_1_id) {
        case SCRIPT_TYPE_INT:
            if (element_2_id == SCRIPT_TYPE_FLOAT) {
                res->data.num_i = element_1->data.num_i > element_2->data.num_f;
            } else {
                res->data.num_i = element_1->data.num_i > element_2->data.num_i;
            }
            break;
        case SCRIPT_TYPE_FLOAT:
            if (element_2_id == SCRIPT_TYPE_FLOAT) {
                res->data.num_i = element_1->data.num_f > element_2->data.num_f;
            } else {
                res->data.num_i = element_1->data.num_f > element_2->data.num_i;
            }
            break;
        default:
            free(res);
            return NULL;
    }
    res->size = 1;
    return res;
}

Element_t* gte_elements(Element_t* element_1, Element_t* element_2) 
{
    if ((element_1 == NULL) || (element_2 == NULL)) {
        return NULL;
    }
    
    VarType_t element_1_id = (element_1->id & 0x7);
    VarType_t element_2_id = (element_2->id & 0x7);

    if ((element_1_id == SCRIPT_TYPE_INVALID) || (element_2_id == SCRIPT_TYPE_INVALID)) {
        return NULL;
    }
    
    if ((element_1_id >= SCRIPT_TYPE_BOOL) || (element_2_id >= SCRIPT_TYPE_BOOL)) {
        return NULL;
    }
    
    Element_t* res = calloc(1, sizeof(Element_t));
    res->id = SCRIPT_TYPE_BOOL;

    switch (element_1_id) {
        case SCRIPT_TYPE_INT:
            if (element_2_id == SCRIPT_TYPE_FLOAT) {
                res->data.num_i = element_1->data.num_i >= element_2->data.num_f;
            } else {
                res->data.num_i = element_1->data.num_i >= element_2->data.num_i;
            }
            break;
        case SCRIPT_TYPE_FLOAT:
            if (element_2_id == SCRIPT_TYPE_FLOAT) {
                res->data.num_i = element_1->data.num_f >= element_2->data.num_f;
            } else {
                res->data.num_i = element_1->data.num_f >= element_2->data.num_i;
            }
            break;
        default:
            free(res);
            return NULL;
    }
    res->size = 1;
    return res;
}

Element_t* lt_elements(Element_t* element_1, Element_t* element_2) 
{
    if ((element_1 == NULL) || (element_2 == NULL)) {
        return NULL;
    }
    
    VarType_t element_1_id = (element_1->id & 0x7);
    VarType_t element_2_id = (element_2->id & 0x7);

    if ((element_1_id == SCRIPT_TYPE_INVALID) || (element_2_id == SCRIPT_TYPE_INVALID)) {
        return NULL;
    }
    
    if ((element_1_id >= SCRIPT_TYPE_BOOL) || (element_2_id >= SCRIPT_TYPE_BOOL)) {
        return NULL;
    }
    
    Element_t* res = calloc(1, sizeof(Element_t));
    res->id = SCRIPT_TYPE_BOOL;

    switch (element_1_id) {
        case SCRIPT_TYPE_INT:
            if (element_2_id == SCRIPT_TYPE_FLOAT) {
                res->data.num_i = element_1->data.num_i < element_2->data.num_f;
            } else {
                res->data.num_i = element_1->data.num_i < element_2->data.num_i;
            }
            break;
        case SCRIPT_TYPE_FLOAT:
            if (element_2_id == SCRIPT_TYPE_FLOAT) {
                res->data.num_i = element_1->data.num_f < element_2->data.num_f;
            } else {
                res->data.num_i = element_1->data.num_f < element_2->data.num_i;
            }
            break;
        default:
            free(res);
            return NULL;
    }
    res->size = 1;
    return res;
}

Element_t* lte_elements(Element_t* element_1, Element_t* element_2) 
{
    if ((element_1 == NULL) || (element_2 == NULL)) {
        return NULL;
    }
    
    VarType_t element_1_id = (element_1->id & 0x7);
    VarType_t element_2_id = (element_2->id & 0x7);

    if ((element_1_id == SCRIPT_TYPE_INVALID) || (element_2_id == SCRIPT_TYPE_INVALID)) {
        return NULL;
    }
    
    if ((element_1_id >= SCRIPT_TYPE_BOOL) || (element_2_id >= SCRIPT_TYPE_BOOL)) {
        return NULL;
    }
    
    Element_t* res = calloc(1, sizeof(Element_t));
    res->id = SCRIPT_TYPE_BOOL;

    switch (element_1_id) {
        case SCRIPT_TYPE_INT:
            if (element_2_id == SCRIPT_TYPE_FLOAT) {
                res->data.num_i = element_1->data.num_i <= element_2->data.num_f;
            } else {
                res->data.num_i = element_1->data.num_i <= element_2->data.num_i;
            }
            break;
        case SCRIPT_TYPE_FLOAT:
            if (element_2_id == SCRIPT_TYPE_FLOAT) {
                res->data.num_i = element_1->data.num_f <= element_2->data.num_f;
            } else {
                res->data.num_i = element_1->data.num_f <= element_2->data.num_i;
            }
            break;
        default:
            free(res);
            return NULL;
    }
    res->size = 1;
    return res;
}

script_int calculate_element(Element_t** elements, script_int num_elements, Element_t* out)
{
    script_int i = 0;
    script_int important_indx = -1;
    VarType_t highest_computation = SCRIPT_TYPE_INVALID;
    for ( ; i < num_elements - 1; i++) {
        if ((elements[i]->id & 0xF0) > highest_computation) {
            highest_computation = elements[i]->id & 0xF0;
            important_indx = i;
        }
    }
    if (highest_computation && ((important_indx + 1) < num_elements)) {
        Element_t* element_1 = elements[important_indx];
        Element_t* element_2 = elements[important_indx + 1];
        switch (highest_computation) {
            case ADD:
                elements[important_indx] = add_elements(element_1, element_2);
                break;
            case SUBTRACT:
                elements[important_indx] = sub_elements(element_1, element_2);
                break;
            case MULTIPLY:
                elements[important_indx] = mul_elements(element_1, element_2);
                break;
            case DIVIDE:
                elements[important_indx] = div_elements(element_1, element_2);
                break;
            case EQUALILTY:
                elements[important_indx] = cmp_elements(element_1, element_2);
                break;
            case DIFFER:
                elements[important_indx] = cmp_elements(element_1, element_2);
                if (elements[important_indx] != NULL) {
                    elements[important_indx]->data.num_i = !elements[important_indx]->data.num_i;
                }
                break;
            case LESS_THAN:
                elements[important_indx] = lt_elements(element_1, element_2);
                break;
            case LT_EQUALS:
                elements[important_indx] = lte_elements(element_1, element_2);
                break;
            case GREATER_THAN:
                elements[important_indx] = gt_elements(element_1, element_2);
                break;
            case GT_EQUALS:
                elements[important_indx] = gte_elements(element_1, element_2);
                break;
            default:
                elements[important_indx] = NULL;
        }
        if (elements[important_indx] == NULL) {
            free(element_1->name);
            free(element_2->name);
            free(element_1);
            free(element_2);
            return SCRIPT_ERR_NO_VAR;  // TODO --> free in outer func
        } else {
            elements[important_indx]->id |= (element_2->id & 0x70);
        }
        free(element_1->name);
        free(element_2->name);
        free(element_1);
        free(element_2);
        // move all elements to the left
        for (i = important_indx + 1; i < num_elements - 1; i++) {
            elements[i] = elements[i + 1];
        }
        elements[i] = NULL;
        return calculate_element(elements, --num_elements, out);
    } else if (elements[i]->id & 0xF0) {
        // Single element - might be a "not"
        switch (elements[i]->id & 0xF0) {
            case DIFFER:
                // Invert only if boolean
                if ((elements[i]->id & 0xF) != SCRIPT_TYPE_BOOL) {
                    return SCRIPT_ERR_INCOMPATIBLE_OP;
                }
                elements[i]->data.num_i = !elements[i]->data.num_i;
                break;
            default:
                return SCRIPT_ERR_INCOMPATIBLE_OP;
        }
        return SCRIPT_OK;
    } else {
        out->data = elements[0]->data;
        out->id = elements[0]->id;
        out->size = elements[0]->size;
        return SCRIPT_OK;
    }
}

script_int parse_list_element(Element_t* main_element, Element_t* list_element, Element_t* out)
{
    // Look for element in 
    Element_t* index_element = calloc(1, sizeof(Element_t));
    script_int ret = get_element(main_element, index_element, 2);
    if (ret < 0) {
        free(index_element);
        SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
    } else if (ret != BRACKET_SQUARE_CLOSE) {
        free(index_element);
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    }
    if (index_element->id != SCRIPT_TYPE_INT) {
        free(index_element);
        SCRIPT_RETURN(SCRIPT_ERR_LIST, (Script_t*)main_element->data.ptr);
    }

    // Now we have the index - let's find the list!
    Element_t* found_element = get_key(list_element->name, global_variable_dict);

    if (found_element == NULL) {
        free(index_element);
        SCRIPT_RETURN(SCRIPT_ERR_NO_VAR, (Script_t*)main_element->data.ptr);
    } else if (found_element->id != SCRIPT_TYPE_LIST) {
        free(index_element);
        SCRIPT_RETURN(SCRIPT_ERR_LIST, (Script_t*)main_element->data.ptr);
    } else if (index_element->data.num_i >= found_element->size) {
        free(index_element);
        SCRIPT_RETURN(SCRIPT_ERR_LIST, (Script_t*)main_element->data.ptr);
    } else if (index_element->data.num_i < 0) {
        index_element->data.num_i = found_element->size + index_element->data.num_i;
    }

    Element_t* element = &((Element_t*)found_element->data.ptr)[index_element->data.num_i];

    free(index_element);

    if (element->name != NULL) {
        strncpy(out->name, element->name, DEFAULT_BUFF_SIZE);
    }
    out->id = element->id;
    out->size = element->size;
    out->data = element->data;

    return 1;
}

script_int parse_list(Element_t* main_element, Element_t* out) 
{
    // We are in square brackets
    // Parse elements until square brackets
    out->size = 0;
    out->id = SCRIPT_TYPE_LIST;
    script_int ret;

    Element_t* list = calloc(8, sizeof(Element_t));
    script_int list_size = 8;

    do {
        if (out->size >= list_size) {
            list_size += 8;
            list = realloc(list, list_size * sizeof(Element_t));
        }
        ret = get_element(main_element, &list[out->size++], 2);
    } while (ret == COMMA);

    if (out->size < list_size) {
        out->data.ptr = realloc(list, out->size * sizeof(Element_t));
    }

    SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
}

// Gather all elements until either comma, semicolon or bracket end
// Then calculate the result, and set out to the resulting element
script_int get_element(Element_t* main_element, Element_t* out, char in_brackets)
{
    script_int count = 0;  // Number of elements
    script_int tot_size = 8;
    script_int str_len = 0;
    script_int ret;
    
    Keyword_t state = INVALID_KEYWORD;
    Element_t* current_element = calloc(1, sizeof(Element_t));
    Element_t** all_elements = calloc(8, sizeof(Element_t*));
    current_element->name = calloc(DEFAULT_BUFF_SIZE, sizeof(char));

    while (state <= INVALID_KEYWORD) {
        // printf("State: %u\n", state);
        //getchar();
        switch (state) {    
            // TODO: Make this actually just space -> separate state for ; and \n
            case BRACKET_SQUARE_OPEN:
                // TODO: Starting list - create a new list
                if (current_element->id == INVALID_KEYWORD) {
                    ret = parse_list(main_element, current_element);
                } else {
                    // Index into list
                    ret = parse_list_element(main_element, current_element, current_element);
                }
                if (ret < 0) {
                    SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
                }
                state = current_element->id;
                goto state_loop;
            case BRACKET_SQUARE_CLOSE:
                if (in_brackets != 2) {
                    SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
                }
                goto parse;
            case END_EXPRESSION:
                if (in_brackets == 1) {
                    SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
                }
                goto parse;
            case BRACKET_STD_CLOSE:
                if (!in_brackets) {
                    SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
                }
                goto parse;
            case COMMA:
                // TODO: Free memory!!!
                parse:
                ret = parse_element(current_element);
                
                if (ret < 0) {
                    // TODO: Free memory!
                    SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
                }
                all_elements[count++] = current_element;
                goto calculate;
            case LOGIC_GT:
                // Check for equals after
                logic_gt_check:
                if (get_next((Script_t*)main_element->data.ptr, SCRIPT_TRUE) == '=') {
                    get_next((Script_t*)main_element->data.ptr, SCRIPT_FALSE);
                    state = LOGIC_GTE;
                }
                goto add_element;
            case LOGIC_LT:
                // Check for equals after
                logic_lt_check:
                if (get_next((Script_t*)main_element->data.ptr, SCRIPT_TRUE) == '=') {
                    get_next((Script_t*)main_element->data.ptr, SCRIPT_FALSE);
                    state = LOGIC_LTE;
                }
                goto add_element;
            case EQUALS:
                equals_check:
                if (get_next((Script_t*)main_element->data.ptr, SCRIPT_FALSE) != '=') {
                    SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
                } else {
                    goto add_element;
                }
            case EXCLAMATION:
                exclamation_check:
                if (get_next((Script_t*)main_element->data.ptr, SCRIPT_FALSE) == '=') {
                    state = LOGIC_NE;
                    goto add_element;
                } else {
                    SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
                }
            case BRACKET_STD_OPEN:
                // Get the element within the bracket
                state = BRACKET_STD_CLOSE;
                if (current_element->id == UNKOWN_STR) {
                    // Run function
                    strncpy(main_element->name, current_element->name, DEFAULT_BUFF_SIZE);
                    free(current_element->name);
                    free(current_element);
                    current_element = NULL;
                    ret = function_statement(main_element, &current_element);
                    if (ret < 0) {
                        SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
                    }
                    if (current_element->size < 1) {
                        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
                    }
                } else {
                    ret = get_element(main_element, current_element, SCRIPT_TRUE);
                    // Check it is valid
                    if (ret < 0) {
                        // TODO: Free memory!
                        SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
                    }
                }
                // At this point, if we are in brackets there are two possibilities:
                // a) There is a comma or end bracket -> this tells us to add the current element as is
                // b) There is an expression -> We need to add more elements
                char c = get_next((Script_t*)main_element->data.ptr, SCRIPT_FALSE);
                // TODO: Use the actual loop
                switch (c) {
                    case '\0':
                        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
                    case ')':
                        // End expression
                        if (!in_brackets) {
                            SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
                        }
                        all_elements[count++] = current_element;
                        goto calculate;
                    case '\n':
                    case ';':
                        if (in_brackets) {
                            SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
                        }
                        
                        all_elements[count++] = current_element;
                        goto calculate;
                    case '+':
                        state = ADDITION;
                        break;
                    case '-':
                        state = SUBTRACTION;
                        break;
                    case '/':
                        state = FWD_SLASH;
                        break;
                    case '*':
                        state = MULTIPLICATION;
                        break;
                    case '>':
                        state = GREATER_THAN;
                        goto logic_gt_check;
                    case '<':
                        state = LESS_THAN;
                        goto logic_lt_check;
                    case '=':
                        state = EQUALS;
                        goto equals_check;
                    case '!':
                        state = EXCLAMATION;
                        goto exclamation_check;
                    default:
                        SCRIPT_RETURN(SCRIPT_ERR_INCOMPATIBLE_OP, (Script_t*)main_element->data.ptr);
                }
            case ADDITION:
            case SUBTRACTION:
                if (current_element->id == INVALID_KEYWORD) {
                    // If we don't have a number yet, we set it to negative
                    current_element->name[0] = '-';
                    current_element->size = 1;
                    state = current_element->id;
                    goto state_loop;
                }
            case MULTIPLICATION:
            case FWD_SLASH:
                // Save element in all_elements, then create a new one.
                // Computation is done after all elements have been found.
                add_element:
                ret = parse_element(current_element);
                if (ret == SCRIPT_FALSE) {
                    SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
                }
                // TODO: Operator should be stored on the next element, not this element
                // This allows the use of !() operator and should allow for error on rogue end operator
                current_element->id |= get_operator(state);
                all_elements[count++] = current_element;
                if (count >= tot_size) {
                    all_elements = realloc(all_elements, sizeof(Element_t) * ++tot_size);
                }
                current_element = calloc(1, sizeof(Element_t));
                current_element->id = INVALID_KEYWORD;
                current_element->name = calloc(DEFAULT_BUFF_SIZE + 1, sizeof(char));
                state = INVALID_KEYWORD;
                break;
            case LITERAL_STR:
                // String has started
                // Copy from buffer into data
                str_len = DEFAULT_BUFF_SIZE + 1;
                current_element->id = SCRIPT_TYPE_STRING;
                current_element->data.ptr = calloc(DEFAULT_BUFF_SIZE + 1, sizeof(char));      // Start with 16 + 1 for '\0'
                break;
            case LITERAL_STR_BUFF_FULL:
                strncpy(((char*)current_element->data.ptr + (str_len - DEFAULT_BUFF_SIZE - 1)), current_element->name, current_element->size);
                str_len += DEFAULT_BUFF_SIZE;
                current_element->data.ptr = realloc(current_element->data.ptr, str_len * sizeof(char));
                state = LITERAL_STR;
                break;
            case LITERAL_STR_END:
                strncpy(((char*)current_element->data.ptr + (str_len - DEFAULT_BUFF_SIZE - 1)), current_element->name, current_element->size);
                str_len += current_element->size;
                *(char*)((char*)current_element->data.ptr + (str_len  - DEFAULT_BUFF_SIZE - 1)) = '\0';
                current_element->size = str_len - DEFAULT_BUFF_SIZE - 1;
                state = SCRIPT_TYPE_STRING;
                current_element->id = SCRIPT_TYPE_STRING;
                goto state_loop;
            case LITERAL_BOOL:
            case LITERAL_CHAR:
            case LITERAL_FLOAT:
            case LITERAL_INT:
            case LITERAL_LIST:
            case INVALID_KEYWORD:
                break;
            default:
                SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
        }
        // Change states -> Do something with the element!
        memset(current_element->name, 0, current_element->size);
        current_element->size = 0;
        current_element->id = state;

        state_loop:

        while (state == current_element->id) {
            // printf("In state: %u\nPtr: %p\n", state, main_element->data);
            //getchar();
            state = read_next((Script_t*)main_element->data.ptr, current_element);
        }
    }
    if (state > INVALID_KEYWORD) {
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    }
    // Calculate
    calculate:
    ret = calculate_element(all_elements, count, out);
    free(all_elements);
    if (ret < 0) {
        SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
    }
    // TODO: Free!!!
    // We have all the elements and the count of them!!!
    SCRIPT_RETURN(state, (Script_t*)main_element->data.ptr);
}

script_int run_custom_func(Element_t* main_element, Element_t* func, Args_t* args)
{
    // Step 1: Save current position in file to return back to
    script_int current_pos = get_file_pos((Script_t*)main_element->data.ptr);

    // Step 2: Jump to function position
    script_int res = jump_to_pos((Script_t*)main_element->data.ptr, func->data.num_i);
    if (res <= 0) {
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    }

    // Step 3: Assign local variables
    for (script_int i = 0; i < (func->size & 0xF); i++) {
        // Read each string until space or comma
        if ((i > 0) && get_next((Script_t*)main_element->data.ptr, SCRIPT_FALSE) != ',') {
            SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
        }
        char* name_buff = calloc(DEFAULT_BUFF_SIZE + 1, sizeof(char));
        parse_name((Script_t*)main_element->data.ptr, name_buff);
        args->input_args[i].name = name_buff;
    }

    if (get_next((Script_t*)main_element->data.ptr, SCRIPT_FALSE) != ')') {
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    }

    if (get_next((Script_t*)main_element->data.ptr, SCRIPT_FALSE) != '{') {
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    } else {
        // Jump to start
        get_next((Script_t*)main_element->data.ptr, SCRIPT_TRUE);
    }

    // Step 3: Set local variables
    local_variable_dict = args->output_args;
    local_variable_size = (func->size & 0xF) + (func->size >> 4);
    return_count = func->size >> 4;

    // Step 4: Run as a normal function
    res = run(main_element->data.ptr, SCRIPT_TRUE);

    if (((func->size >> 4) > 0) && (res != 2)) { // 2 is returned from 'return' keyword
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    }

    // Step 5: Reset variables and file position
    local_variable_dict = NULL;
    local_variable_size = 0;
    return_count = 0;

    res = jump_to_pos((Script_t*)main_element->data.ptr, current_pos);
    if (res <= 0) {
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    }
    
    return 1;
}

script_int run_function(Element_t* main_element, Element_t* func, Element_t** out)
{
    Element_t* all_args = calloc((func->size & 0xF) + (func->size >> 4), sizeof(Element_t));
    Args_t args;
    script_int output_arg_count = (func->size >> 4);
    if (output_arg_count <= 0) {
        args.output_args = NULL;
    } else if (output_arg_count == 1) {
        args.output_args = all_args;
    } else {
        args.output_args = calloc(1, sizeof(Element_t));
        args.output_args->id = SCRIPT_TYPE_LIST;
        args.output_args->size = output_arg_count;
        args.output_args->data.ptr = all_args;
    }
    args.input_args = &all_args[output_arg_count];

    script_int count = 0;  // Used to keep track of # of args
    script_int res = BRACKET_STD_CLOSE;
    
    while (count < (func->size & 0xF)) {
        // Get element
        res = get_element(main_element, &args.input_args[count++], SCRIPT_TRUE);
        if (res < 0) {
            SCRIPT_RETURN(res, (Script_t*)main_element->data.ptr);
        }
    }

    if (res != BRACKET_STD_CLOSE) {
        // If haven't finished the function
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    }
    
    if (count == 0) {
        if (get_next((Script_t*)main_element->data.ptr, SCRIPT_FALSE) != ')') {
            SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
        }
    }

    script_int ret;

    if (func->id == FUNC_BUILTIN) {
        ret = ((ScriptFunc_t)func->data.ptr)(&args);
    } else if (func->id == FUNC_CUSTOM) {
        ret = run_custom_func(main_element, func, &args);
    } else {
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    }
    
    *out = args.output_args;
    SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
}

script_int parse_function(Element_t* main_element)
{
    Keyword_t state;
    Element_t* search_element = calloc(1, sizeof(Element_t));
    char search_str[DEFAULT_BUFF_SIZE];
    search_element->name = search_str;
    
    // Step 1: Save the name of the function
    Element_t* func_element = calloc(1, sizeof(Element_t));
    func_element->name = calloc(main_element->size, sizeof(char));
    strncpy(func_element->name, main_element->name, main_element->size + 1);
    func_element->data.num_i = get_file_pos((Script_t*)main_element->data.ptr);
    
    // Step 2: Check for number of variables
    do {
        search_element->id = INVALID_KEYWORD;
        
        do {
            state = read_next((Script_t*)main_element->data.ptr, search_element);
        } while (state == INVALID_KEYWORD);

        if (state == BRACKET_STD_CLOSE) {
            break;
        } else if (state != UNKOWN_STR) {
            SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
        }

        func_element->size++;
        search_element->size = 0;
        state = read_next((Script_t*)main_element->data.ptr, search_element);
    } while (state == COMMA);

    if (state != BRACKET_STD_CLOSE) {
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    }

    
    if (func_element->size > 0xF) {
        free(search_element);
        free(func_element);
        SCRIPT_RETURN(SCRIPT_ERR_TOO_MANY_ELEMENTS, (Script_t*)main_element->data.ptr);
    }

    // Step 3: Check for open curly brackets
    if (get_next((Script_t*)main_element->data.ptr, SCRIPT_FALSE) != '{') {
        free(search_element);
        free(func_element);
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    }

    // Step 4: Check number of return variables
    script_int depth = jump_to_s((Script_t*)main_element->data.ptr, "return", 6);

    if (depth < 0) {
        free(search_element);
        free(func_element);
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    }

    script_int ret = count_until((Script_t*)main_element->data.ptr, ',', '\n');
    if (ret < 0) {
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    } else if (ret > 0xF) {
        SCRIPT_RETURN(SCRIPT_ERR_TOO_MANY_ELEMENTS, (Script_t*)main_element->data.ptr);
    }
    func_element->size += (ret + 1) << 4;

    // Step 5: Save and jump out of element
    func_element->id = FUNC_CUSTOM;
    put_function(func_element);
    return jump_out((Script_t*)main_element->data.ptr, depth + 1);
}

script_int function_statement(Element_t* main_element, Element_t** out)
{
    script_int ret = 0;
    Element_t* func = get_key(main_element->name, ctrl_flow_dict);
    if (func != NULL) {
        // Run builtin function
        ret = ((ScriptCtrlFunc_t)func->data.ptr)(main_element);
        SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
    }
    func = get_key(main_element->name, function_dict);
    if (func != NULL) {
        return run_function(main_element, func, out);
    } else {
        // Must be trying to create new function - how exciting!
        // Need to gather the variables 
        ret = parse_function(main_element);
        if (ret < 0) {
            SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
        }
        SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
    }
}

script_int script_while(Element_t* main_element)
{
    // We know we already have 'while' and brackets. The nex
    script_int eval = SCRIPT_TRUE;
    script_int pos = get_file_pos((Script_t*)main_element->data.ptr);

    // Step 1: Get the resulting element
    if ((main_element == NULL)) {
        SCRIPT_RETURN(-1, (Script_t*)main_element->data.ptr);
    }
    Element_t new_element;

    while (1) {
        int ret = get_element(main_element, &new_element, SCRIPT_TRUE);

        if (ret < 0) {
            SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
        }
        
        if (get_next((Script_t*)main_element->data.ptr, SCRIPT_FALSE) != '{') {
            SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
        }
    
        switch (new_element.id & 0x7) {
            case SCRIPT_TYPE_BOOL:
            case SCRIPT_TYPE_INT:
                eval = new_element.data.num_i != 0;
                break;
            case SCRIPT_TYPE_FLOAT:
                eval = new_element.data.num_f != 0;
                break;
            case SCRIPT_TYPE_LIST:
                eval = new_element.size > 0;
                break;
            case SCRIPT_TYPE_STRING:
                eval = (new_element.size > 0) && ((char*)new_element.data.ptr)[0] != '\0';
                break;
            default:
                SCRIPT_RETURN(SCRIPT_ERR_INCOMPATIBLE_OP, (Script_t*)main_element->data.ptr);
        }

        if (!eval) {
            break;
        }

        ret = run(main_element->data.ptr, SCRIPT_TRUE);
        
        if (ret < 0) {
            SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
        } else if (ret == 2) {
            jump_out((Script_t*)main_element->data.ptr, 1);
            return 5;
        }
        // Go back to the start of the while loop
        ret = jump_to_pos((Script_t*)main_element->data.ptr, pos);
        if (ret < 0) {
            SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
        }
    }
    
    return jump_out((Script_t*)main_element->data.ptr, 1);
}

script_int script_if(Element_t* main_element)
{
    // We know we already have 'if', and we have the brackets. The next element we expect is the curly open brackets.
    // If we don't see curly open brackets, return error.
    script_int eval = SCRIPT_TRUE;

    // Step 1: Get the resulting element
    if ((main_element == NULL)) {
        SCRIPT_RETURN(SCRIPT_ERR_PARSE, (Script_t*)main_element->data.ptr);
    } else if (main_element->id == ELSE) {
        goto run_if_eval;
    } 
    
    Element_t new_element;
    int ret = get_element(main_element, &new_element, SCRIPT_TRUE);
    
    if (ret < 0) {
        return ret;
    }
    if (get_next((Script_t*)main_element->data.ptr, SCRIPT_FALSE) != '{') {
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    }
    if (main_element->id == INVALID_KEYWORD) {
        eval = SCRIPT_FALSE;
        goto run_if_eval;
    }

    switch (new_element.id & 0x7) {
        case SCRIPT_TYPE_BOOL:
        case SCRIPT_TYPE_INT:
            eval = new_element.data.num_i != 0;
            break;
        case SCRIPT_TYPE_FLOAT:
            eval = new_element.data.num_f != 0;
            break;
        case SCRIPT_TYPE_STRING:
            eval = new_element.size > 0;
            eval &= ((char*)new_element.data.ptr)[0] != '\0';
            break;
        default:
            SCRIPT_RETURN(SCRIPT_ERR_INCOMPATIBLE_OP, (Script_t*)main_element->data.ptr);
    }

    run_if_eval:
    // Eval tells us that if to run or not
    if (eval) {
        // Run script inside the if!!!
        ret = run(main_element->data.ptr, SCRIPT_TRUE);
        if (ret < 0) {
            SCRIPT_RETURN(ret, (Script_t*)main_element->data.ptr);
        } else if (ret == 2) {
            jump_out((Script_t*)main_element->data.ptr, 1);
            return 5;
        }
        main_element->id = INVALID_KEYWORD;
    } else if (!jump_out((Script_t*)main_element->data.ptr, 1)) {
        // Couldn't find end of if
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    }

    // Then check for 'else' keyword
    if (get_next((Script_t*)main_element->data.ptr, SCRIPT_TRUE) == 'e') {
        // Possibly 'else' statement
        fgets(main_element->name, 5, (FILE*)main_element->data.ptr);
        if (strncmp(main_element->name, "else", 4) != 0) {
            // Not else
            SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
        }
    } else {
        // Finished if
        return SCRIPT_OK;
    }
    
    // If 'else' then check for 'if'
    if (get_next((Script_t*)main_element->data.ptr, SCRIPT_TRUE) == 'i') {
        // Possibly 'if' statement
        fgets(main_element->name, 3, (FILE*)main_element->data.ptr);
        if (strncmp(main_element->name, "if", 2) == 0) {
            // If -> Run recursively
            // Check for '('
            if (get_next((Script_t*)main_element->data.ptr, SCRIPT_FALSE) != '(') {
                SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
            }
            return script_if(main_element);
        } else {
            // Invalid string -> Return error
            SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
        }
    } else {
        // Only 'else' -> Check for '{' and run if
        if (get_next((Script_t*)main_element->data.ptr, SCRIPT_FALSE) != '{') {
            SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
        }
        if ((main_element->id == ELSE) || (main_element->id == INVALID_KEYWORD)) {
            // Don't run
            return jump_out((Script_t*)main_element->data.ptr, 1);
        } else {
            main_element->id = ELSE;
            return script_if(main_element);
        }
    }
}

script_int script_return(Element_t* main_element) {
    // The return keyword should return return_count number of elements
    script_int count = 0;
    script_int ret = 0;
    while (count < return_count) {
        ret = get_element(main_element, &local_variable_dict[count++], SCRIPT_TRUE);
        if (ret < 0) {
            SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
        }
    }
    
    if (ret != BRACKET_STD_CLOSE) {
        // If haven't finished the function
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, (Script_t*)main_element->data.ptr);
    }

    return 5;
}

script_int run(Script_t* file, script_int in_func) {
    char read_buff[DEFAULT_BUFF_SIZE]; // TODO #define size
    Element_t current_element = {
        .name = read_buff,
        .id = INVALID_KEYWORD,
        .size = 0,
        .data.ptr = file
    };
    Keyword_t state = INVALID_KEYWORD;

    script_int err = 0;

    while (state <= INVALID_KEYWORD) {
        // printf("State: %u\n", state);
        // //getchar();
        switch (state) {
            case BRACKET_CURL_CLOSE:
                if (in_func) {
                    return 1;
                } else {
                    SCRIPT_RETURN(SCRIPT_ERR_FORMAT, file);
                }
            case FWD_SLASH:
                if (get_next((Script_t*)file, SCRIPT_FALSE) == '/') {
                    // Comment
                    jump_to_next(file);
                    state = INVALID_KEYWORD;
                    break;
                } else {
                    SCRIPT_RETURN(SCRIPT_ERR_FORMAT, file);
                }
            case EQUALS:
                // Go to new function which gets the value -> returns error if doesn't find appropriate value
                Element_t* new_element = calloc(1, sizeof(Element_t));
                Element_t* found_element = get_key(current_element.name, global_variable_dict);
                if (found_element == NULL) {
                    // OMG We have a new variable!!!
                    found_element = new_element;
                    found_element->name = calloc(DEFAULT_BUFF_SIZE + 1, sizeof(char));
                    strncpy(found_element->name, current_element.name, DEFAULT_BUFF_SIZE);
                } else if (found_element->id & SCRIPT_TYPE_CONST) {
                    SCRIPT_RETURN(SCRIPT_ERR_FORMAT, file);      // Trying to change const value
                }
                err = get_element(&current_element, new_element, SCRIPT_FALSE);
                if (err < 0) {
                    SCRIPT_RETURN(err, file);
                }
                if (found_element == new_element) {
                    // New variable -> put into array
                    put_element(new_element);
                } else {
                    found_element->data = new_element->data;
                    found_element->size = new_element->size;
                    found_element->id = new_element->id;        // Int and bool can change
                    free(new_element);
                }
                state = INVALID_KEYWORD;
                break;
            case BRACKET_STD_OPEN:
                if (current_element.id != UNKOWN_STR) {
                    SCRIPT_RETURN(SCRIPT_ERR_FORMAT, file);
                }
                Element_t* output_elements;
                err = function_statement(&current_element, &output_elements);
                if (err < 0) {
                    // Error code
                    close_script(file);
                    return err;
                }
                if (err == 5) {
                    // Jump out of function and return
                    if (in_func) {
                        jump_out(file, 1);
                        return 2;
                    } else {
                        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, file);
                    }
                }
                state = INVALID_KEYWORD;
                break;
            case BRACKET_SQUARE_OPEN:
                // If we have an unkown string, we are setting an element within a list
                // Need to ensure we have an equal sign after the index
                // TODO
            default:
                state = INVALID_KEYWORD;
                break;
        }
        // Change states -> Do something with the element!
        memset(read_buff, 0, sizeof(read_buff));
        current_element.size = 0;
        current_element.id = state;

        while (state == current_element.id) {
            // printf("In state: %u\n", state);
            //getchar();
            state = read_next(file, &current_element);
        }
    }
    if (in_func) {
        SCRIPT_RETURN(SCRIPT_ERR_FORMAT, file);
    }
    close_script(file);
    return 0;
}

script_int main(script_int argc, char * argv[]) {
    
    // clock_t start, end;
    // double cpu_time_used;

    // start = clock();
    
    char* script_path = NULL;
    if (argc < 2) {
        // No arguments given
        // printf("Error - No file provided\n\n");
        // printf("Usage: %s [filepath]\n", argv[0]);
        // return 1;
        script_path = "scripts/test.z";
    } else {
        script_path = argv[1];
    }

    Script_t* file = open_script(script_path);
    if (file == NULL) {
        printf("Error - Could not open file %s\n", argv[1]);
        SCRIPT_RETURN(SCRIPT_ERR_NO_FILE, file);
    }
    run(file, SCRIPT_FALSE);

    // end = clock();

    // cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    // double milliseconds = cpu_time_used * 1000.0;

    // printf("Time elapsed: %.2f milliseconds\n", milliseconds);
    return 0;
}