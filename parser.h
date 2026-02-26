#ifndef ZSCRIPT_PARSER_H
#define ZSCRIPT_PARSER_H

#include <stdio.h>
#include <stdint.h>

#include "script_stddefs.h"


#define INVALID_KEYWORD         NUM_KEYWORDS

char parse_char(Script_t* script);
Script_t* open_script(const char* path);
void close_script(Script_t* script);
Keyword_t read_next(Script_t* script, Element_t* element);
void jump_to_next(Script_t* script);
char get_next(Script_t* script, int peek);
script_int jump_to(Script_t* script, char to);
script_int jump_out(Script_t* script, script_int level);
uint32_t get_file_pos(Script_t* script);
script_int jump_to_s(Script_t* script, const char* search, script_int len);
script_int count_until(Script_t* script, const char count_c, const char end_c);
script_int jump_to_pos(Script_t* script, uint32_t pos);
script_int parse_name(Script_t* script, char* name_out);
script_int get_line_number(Script_t* script);
script_int print_line(Script_t* script, int ln);
void print_n_chars(char c, script_int len);

#endif  // ZSCRIPT_PARSER_H