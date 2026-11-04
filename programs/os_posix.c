/**
 * @file
 * @brief POSIX platform entry point for the CLI OS layer
 * @copyright GPL-2.0
 */

/*
 * These macros tell the system headers which functions and constants to make
 * available, see feature_test_macros(7).
 * _POSIX_C_SOURCE enables fileno(), snprintf() when building as C89.
 * _DEFAULT_SOURCE enables MAP_ANON on glibc.
 * _DARWIN_C_SOURCE enables MAP_ANON on MacOS.
 */
#ifndef _DEFAULT_SOURCE
#  define _DEFAULT_SOURCE 1
#endif
#ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 200809L
#endif
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#  define _DARWIN_C_SOURCE 1
#endif

#include "os.c"

#include <dirent.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../lib/common/compiler.h"


static int os_set_binary_mode(FILE *stream UNUSED)
{
	/* POSIX streams do not distinguish text from binary. */
	return 0;
}


static int os_isatty(FILE *stream)
{
	return isatty(fileno(stream));
}


static void *os_alloc(size_t size)
{
	void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	return p == MAP_FAILED ? NULL : p;
}


int os_is_directory(const char *path)
{
	struct stat st;

	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}


int os_is_regular_file(const char *path)
{
	struct stat st;

	return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}


int os_directory_is_empty(const char *path)
{
	DIR *dir;
	struct dirent *entry;
	int read_error;

	dir = opendir(path);
	if (!dir)
		return 0;

	errno = 0;
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
			(void)closedir(dir);
			return 0;
		}
	}
	read_error = errno;

	if (closedir(dir) != 0)
		return 0;

	return read_error == 0;
}


int os_make_directory(const char *path)
{
	int r = mkdir(path, 0777);

	if (r != 0 && errno == EEXIST) {
		if (os_is_directory(path))
			return 0;
		errno = EEXIST;
	}
	return r;
}


int os_remove(const char *path)
{
	return remove(path);
}
