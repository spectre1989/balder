#pragma once

#include "types.h"


bool string_starts_with(const char* str, const char* search);
bool string_ends_with(const char* str, const char* search);
int32 string_copy(char* dst, int32 dst_size, const char* src);
int32 string_len(const char* str);
char* string_copy(const char* src);
bool char_is_whitespace(char c);
bool string_read_line(const char** str, const char* str_end);
void string_copy_substring(char* dst, const char* src, uint32 count);
bool string_equals(const char* a, const char* b);