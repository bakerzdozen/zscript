#include "script_builtins.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char* get_type_str(VarType_t type)
{
    switch (type) {
        case SCRIPT_TYPE_BOOL:
            return "Bool";
        case SCRIPT_TYPE_INT:
            return "Integer";
        case SCRIPT_TYPE_FLOAT:
            return "Float";
        case SCRIPT_TYPE_STRING:
            return "String";
        case SCRIPT_TYPE_LIST:
            return "List";
        case SCRIPT_TYPE_TYPE:
            return "Type";
        default:
            return "Invalid";
    }
}

static script_int print_element(const Element_t* element)
{
    switch (element->id & (0x7)) {
        case SCRIPT_TYPE_BOOL:
            printf("%s", (element->data.num_i == 0) ? "False" : "True");
            break;
        case SCRIPT_TYPE_INT:
            printf("%i", element->data.num_i);
            break;
        case SCRIPT_TYPE_FLOAT:
            printf("%f", element->data.num_f);
            break;
        case SCRIPT_TYPE_STRING:
            printf("%s", (char*)element->data.ptr);
            break;
        case SCRIPT_TYPE_TYPE:
            printf("%s", get_type_str(element->data.num_i));
            break;
        case SCRIPT_TYPE_LIST:
            script_int i;
            
            Element_t* list = (Element_t*)element->data.ptr;
            
            putchar('[');

            for (i = 0; i < (element->size - 1); i++) {
                print_element(&list[i]);
                putchar(',');
            }
            
            if (i < element->size) {
                print_element(&list[i]);
            }

            putchar(']');
            break;
        default:
            return -1;
    }

    return SCRIPT_OK;
}

script_int script_print(Args_t* args)
{
    script_int ret = print_element(&args->input_args[0]);
    
    if (ret != SCRIPT_OK) {
        return ret;
    }

    printf("\n");
    return 0;
}

script_int script_size(Args_t* args)
{
    args->output_args->id = SCRIPT_TYPE_INT;
    args->output_args->size = 1;

    switch (args->input_args->id & (0x7)) {
        case SCRIPT_TYPE_BOOL:
        case SCRIPT_TYPE_INT:
        case SCRIPT_TYPE_FLOAT:
        case SCRIPT_TYPE_STRING:
        case SCRIPT_TYPE_LIST:
            args->output_args->data.num_i = args->input_args[0].size;
            break;
        default:
            return -1;
    }
    return 0;
}

script_int script_type(Args_t* args)
{
    args->output_args->id = SCRIPT_TYPE_TYPE;
    args->output_args->size = 1;

    args->output_args->data.num_i = args->input_args->id & 0x7;

    return 1;
}

script_int script_readline(Args_t* args)
{
    args->output_args->id = SCRIPT_TYPE_STRING;
    
    int size = DEFAULT_BUFF_SIZE + 1;
    char* buff = calloc(size, sizeof(char));

    while (fgets(buff + size - (DEFAULT_BUFF_SIZE + 1), DEFAULT_BUFF_SIZE + 1, stdin)) {
        while (buff[size - 2] == '\0') {
            size--;
        }
        if (buff[size - 2] == '\n') {
            buff[--size - 1] = '\0';
            args->output_args->size = size - 1;
            args->output_args->data.ptr = realloc(buff, size * sizeof(char));
            return 1;
        }
        size += DEFAULT_BUFF_SIZE;
        buff = realloc(buff, size * sizeof(char));
        memset(buff + (size - DEFAULT_BUFF_SIZE), '\0', DEFAULT_BUFF_SIZE);
    }

    args->output_args->data.ptr = buff;
    args->output_args->size = size - 1;

    return 1;
}