/**
 * @file
 * @brief MinGW-w64 platform entry point for the CLI OS layer (ASCII paths)
 * @copyright GPL-2.0
 */

#include "os.c"

#include <windows.h>
#include <shlwapi.h>
#include <direct.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>

/* This is pure Windof fun! */
static int os_set_binary_mode(FILE *stream)
{
	/* CRT text mode translates CRLF and treats ^Z as EOF. */
	return _setmode(_fileno(stream), _O_BINARY) == -1;
}


static int os_isatty(FILE *stream)
{
	return _isatty(_fileno(stream));
}


static void *os_alloc(size_t size)
{
	return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}


int os_is_directory(const char *path)
{
	DWORD attr = GetFileAttributesA(path);
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}


int os_is_regular_file(const char *path)
{
	DWORD attr = GetFileAttributesA(path);
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
	       (attr & FILE_ATTRIBUTE_DEVICE) == 0;
}


int os_directory_is_empty(const char *path)
{
	/* Shlwapi handles the directory check and ignores "." and "..". */
	return PathIsDirectoryEmptyA(path);
}


int os_make_directory(const char *path)
{
	int r = _mkdir(path);

	if (r != 0 && errno == EEXIST) {
		if (os_is_directory(path))
			return 0;
		errno = EEXIST;
	}
	return r;
}


int os_remove(const char *path)
{
	return os_is_directory(path) ? _rmdir(path) : remove(path);
}
