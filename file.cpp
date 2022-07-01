#include "file.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "assert.h"


File read_file(const char* path)
{
	HANDLE file_handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	LARGE_INTEGER file_size;
	GetFileSizeEx(file_handle, &file_size);

	File file = {};
	file.size = file_size.QuadPart;
	file.data = new uint8[file_size.QuadPart];

	DWORD bytes_read;
	const BOOL success = ReadFile(file_handle, file.data, file_size.QuadPart, &bytes_read, nullptr);
	assert(success);

	return file;
}