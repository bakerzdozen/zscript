#include "parser.h"
#include <string.h>

struct _script {
    FILE* file;
};

Script_t* open_script(const char* path) 
{
    return (Script_t*)fopen(path, "rb");
}

void close_script(Script_t* script)
{
    fclose((FILE*)script);
}

char parse_char(Script_t* script) {
    return fgetc((FILE*)script);
}

// Returns '\0' if element-> size >= 16 or end of file or new character found
// Else returns the char

// Return value is the next keyword
// TODO: Improve this by not returning until need to (i.e. read str literal, str unkown etc until get int, str, etc then find next char -> if +,-,*,/ then append that and parse before returning)
Keyword_t read_next(Script_t* script, Element_t* element)
{
    int ret = fgetc((FILE*)script);
    // printf("Char: \"%c\" (%u)\n", ret, ret);
    if (ret < ' ') {
        if (ret <= 0) {
            return FILE_END;
        }
        switch (ret) {
            // TODO: Don't just return element ID if potentially number or something --> MAJOR
            // Also maybe allow string literals to include \t etc
            case '\n':
                break;
            case '\r':
            case '\t':
                return element->id;
            default:
                return PARSE_ERR;
        }
    }

    if (element->id == LITERAL_STR) {
        // TODO: new switch statement for escape chars etc?
        // TODO: Allow the string literal "\n"
        if (ret == '"') {
            return LITERAL_STR_END;
        }
        ((char*)element->name)[element->size++] = ret;
        if (element->size >= DEFAULT_BUFF_SIZE) {
            return LITERAL_STR_BUFF_FULL;
        }
        return LITERAL_STR;
    }
        
        
    switch (ret) {
        case '\0':
            // End of file
            return FILE_END;
        case ' ':
            // No keyword has ' '
            return element->id; // Ignore spaces
        case '(':
            // Begin bracket
            return BRACKET_STD_OPEN;
        case '[':
            return BRACKET_SQUARE_OPEN;
        case ']':
            return BRACKET_SQUARE_CLOSE;
        case ')':
            // End bracket
            return BRACKET_STD_CLOSE;
        case '{':
            // Begin function
            return BRACKET_CURL_OPEN;
        case '}':
            // End function
            return BRACKET_CURL_CLOSE;
        case '*':
            // Multiplication
            return MULTIPLICATION;
        case '>':
            return LOGIC_GT;
        case '<':
            return LOGIC_LT;
        case '/':
            // If in outer scope, check for second -- comment
            return FWD_SLASH;
        case '=':
            // Variable assignment
            return EQUALS;
        case '-':
            // Subtraction
            return SUBTRACTION;
        case '+':
            // Addition
            return ADDITION;
        case '.':
            // Possibly double
            if (element->id == LITERAL_INT) {
                element->id = LITERAL_FLOAT;
                if (element->size >= DEFAULT_BUFF_SIZE) {
                    return SIZE_OVERFLOW;
                }
                element->name[element->size++] = ret;
                return LITERAL_FLOAT;
            } else {
                return INVALID_KEYWORD;
            }
        case '"':
            // Literal string
            return LITERAL_STR;
        case ',':
            return COMMA;
        case '\n':
        case ';':
            return END_EXPRESSION;
        case '!':
            return EXCLAMATION;
        default:
            break;
    }
    element->name[element->size++] = ret;
    if (element->size >= DEFAULT_BUFF_SIZE) {
        if (element->id == LITERAL_STR) {
            return LITERAL_STR_BUFF_FULL;
        } else {
            return SIZE_OVERFLOW;
        }
    }
    if ((element->id != UNKOWN_STR) && (ret - '0' < 10)) {
        if (element->id == LITERAL_INT || element->id == LITERAL_FLOAT) {
            return element->id;
        } else {
            element->id = LITERAL_INT;
            return LITERAL_INT;
        }
    }
    // String
    if (element->id == INVALID_KEYWORD) {
        element->id = UNKOWN_STR;
    }
    if (element->id == UNKOWN_STR) {
        return element->id;
    }
    return INVALID_KEYWORD;
}

void jump_to_next(Script_t* script)
{
    char c = getc((FILE*)script);
    while (c != '\n') {
        c = getc((FILE*)script);
        if ((c == '\0') || (c == -1)) {
            return;
        }
    }
    while (c <= ' ') {
        c = getc((FILE*)script);
        if ((c == '\0') || (c == -1)) {
            return;
        }
    }
    ungetc(c, (FILE*)script);
}

char get_next(Script_t* script, int peek)
{
    int c = getc((FILE*)script);
    while (c <= ' ') {
        if (c <= '\0') {
            return -1;
        } else if (c == '\n') {
            return c;
        }
        c = getc((FILE*)script);
    }
    if (peek) {
        ungetc(c, (FILE*)script);
    }
    return (char)c;
}

script_int jump_to(Script_t* script, char to)
{
    int c = getc((FILE*)script);
    while (c != to) {
        if (c <= 0) {
            return SCRIPT_FALSE;
        }
        c = getc((FILE*)script);
    }
    return SCRIPT_TRUE;
}

// Jumps out of current scope
script_int jump_out(Script_t* script, script_int level) 
{
    int c = getc((FILE*)script);

    while (level > 0) {
        if (c <= 0) {
            return -1;
        } else if (c == '{') {
            level++;
        } else if (c == '}') {
            level--;
        }
        c = getc((FILE*)script);
    }
    return 1;
}

uint32_t get_file_pos(Script_t* script)
{
    return ftell((FILE*)script);
}

script_int jump_to_s(Script_t* script, const char* search, script_int len)
{
    script_int pos = 0;
    script_int depth = 0;

    int c;

    while (pos < len) {
        c = getc((FILE*)script);

        if (c == search[pos]) {
            pos++;
        } else if (c <= '\0') {
            return -1;
        } else {
            if (c == '{') {
                depth++;
            } else if (c == '}') {
                depth--;
            }
            pos = 0;
        }
    }

    return depth;
}

script_int count_until(Script_t* script, const char count_c, const char end_c)
{
    int c;
    script_int count = 0;

    do {
        c = getc((FILE*)script);
        if (c == count_c) {
            count++;
        } else if (c <= '\0') {
            return -1;
        }
    } while (c != end_c);

    return count;
}

script_int jump_to_pos(Script_t* script, uint32_t pos)
{
    if (fseek((FILE*)script, pos, SEEK_SET) != 0) {
        return -1;
    } else {
        return 1;
    }
}

script_int parse_name(Script_t* script, char* name_out)
{
    int c;
    int count = 0;
    
    do {
        c = getc((FILE*)script);
        if (c <= '\0') {
            return -1;
        }
    } while (c <= ' ');

    while (1) {
        if (c <= '0') {
            ungetc(c, (FILE*)script);
            return (count > 0);
        }
        if ((c > '9') && (c < 'A')) {
            ungetc(c, (FILE*)script);
            return (count > 0);
        }
        if ((c > 'Z') && (c < '_')) {
            ungetc(c, (FILE*)script);
            return (count > 0);
        }
        if ((c > 'z') || (c == '`')) {
            ungetc(c, (FILE*)script);
            return (count > 0);
        }
        name_out[count++] = c;
        if (count >= DEFAULT_BUFF_SIZE) {
            return -1;
        }
        c = fgetc((FILE*)script);
    }
}

script_int get_line_number(Script_t* script)
{
    if (script == NULL) {
        return -1;
    }

    script_int current_pos = get_file_pos(script);

    if (current_pos < 0) {
        return -1;
    }

    script_int line_no = 1;
    int c;

    jump_to_pos(script, 0);

    for (script_int i = 0; i < current_pos; i++) {
        c = fgetc((FILE*)script);
        if (c <= '\0') {
            return -1;
        }
        if (c == '\n') {
            line_no++;
        }
    }
    return line_no;
}