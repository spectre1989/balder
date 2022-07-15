#include "string.h"


bool string_starts_with(const char* str, const char* search)
{
	while (*search)
	{
		if (*search != *str)
		{
			return false;
		}

		++str;
		++search;
	}

	return true;
}

bool string_ends_with(const char* str, const char* search)
{
	const char* str_iter = str;
	while (*str_iter) 
	{
		++str_iter;
	}
	const int32 str_len = str_iter - str;

	const char* search_iter = search;
	while (*search_iter)
	{
		++search_iter;
	}
	const int32 search_len = search_iter - search;

	if (str_len < search_len)
	{
		return false;
	}

	while (true)
	{
		if (*str_iter != *search_iter)
		{
			return false;
		}

		if (search_iter == search)
		{
			return true;
		}

		--str_iter;
		--search_iter;
	}
}

int32 string_copy(char* dst, int32 dst_size, const char* src)
{
	char* dst_iter = dst;
	while (dst_size > 1 && *src)
	{
		*dst_iter = *src;
		--dst_size;
		++dst_iter;
		++src;
	}
	*dst_iter = 0;

	return dst_iter - dst;
}

int32 string_len(const char* str)
{
	const char* iter = str;
	while (*iter)
	{
		++iter;
	}

	return iter - str;
}

char* string_copy(const char* src)
{
	const int32 len = string_len(src);
	char* dst = new char[len + 1];
	string_copy(dst, len + 1, src);
	return dst;
}

bool char_is_whitespace(char c)
{
	return c == ' ' || c == '\r' || c == '\n' || c == '\t';
}

bool string_read_line(const char** str, const char* str_end)
{
	const char* iter = *str;
	while (true)
	{
		if (iter == str_end || !*iter)
		{
			return false;
		}

		if (*iter == '\n')
		{
			++iter;
			*str = iter;
			return true;
		}

		++iter;
	}
}

void string_copy_substring(char* dst, const char* src, uint32 count)
{
	while (count)
	{
		*dst = *src;
		++dst;
		++src;
		--count;
	}
	*dst = 0;
}

bool string_equals(const char* a, const char* b)
{
	while (true)
	{
		if (*a != *b)
		{
			return false;
		}

		if (!*a)
		{
			return true;
		}

		++a;
		++b;
	}
}