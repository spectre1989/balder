#pragma once

#include "types.h"


struct File
{
	uint64 size;
	uint8* data;
};

File read_file(const char* path);