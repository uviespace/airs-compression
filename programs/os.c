/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2026
 * @brief Common parts of the os API
 * @copyright GPL-2.0
 *
 * Included by one platform source (os_posix.c or os_windows.c).
 */

#include "os.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Private hooks implemented by the selected platform entry point. */
static int os_set_binary_mode(FILE *stream);
static int os_isatty(FILE *stream);
static void *os_alloc(size_t size);

struct arena os_arena(void)
{
	void *p;

	if (OS_ARENA_CAP == 0 || OS_ARENA_CAP > PTRDIFF_MAX)
		arena_oom();

	p = os_alloc(OS_ARENA_CAP);
	if (!p)
		arena_oom();

	return arena_init(p, OS_ARENA_CAP);
}


struct os_load os_read(const char *path, void *buf, uint32_t buf_size)
{
	FILE *fp;
	size_t bytes_read;
	struct os_load r = { 0 };

	assert(path);
	assert(buf);

	if (!strcmp(path, STD_IN_MARK)) {
		fp = stdin;
		if (os_set_binary_mode(fp)) {
			r.status = OS_IOERR;
			return r;
		}
	} else {
		/*
		 * MinGW rejects directories in fopen(), while POSIX fails on read.
		 * Check first so both report OS_IOERR rather than OS_CANTOPEN.
		 */
		if (os_is_directory(path)) {
			errno = EISDIR;
			r.status = OS_IOERR;
			return r;
		}
		fp = fopen(path, "rb");
		if (!fp) {
			r.status = OS_CANTOPEN;
			return r;
		}
	}

	bytes_read = fread(buf, 1, buf_size, fp);
	assert(bytes_read <= buf_size);
	if (bytes_read == buf_size) {
		int c = fgetc(fp);

		if (c != EOF) {
			r.status = OS_TOOBIG;
			(void)ungetc(c, fp);
		}
	}

	if (ferror(fp) && r.status == OS_OK)
		r.status = OS_IOERR;

	if (fp != stdin)
		if (fclose(fp) && r.status == OS_OK)
			r.status = OS_CANTCLOSE;

	r.buffer = buf;
	assert(bytes_read <= INT32_MAX);
	r.size = (uint32_t)bytes_read;
	return r;
}


enum os_status os_save_from_buffer(const char *path, const void *buf, uint32_t buf_size)
{
	FILE *fp;
	size_t bytes_written;
	enum os_status r = OS_OK;

	assert(path);
	assert(buf);

	if (!strcmp(path, STD_OUT_MARK)) {
		fp = stdout;
		if (os_set_binary_mode(fp))
			return OS_IOERR;
	} else {
		fp = fopen(path, "wb");
		if (!fp)
			return OS_CANTOPEN;
	}

	bytes_written = fwrite(buf, 1, buf_size, fp);
	if (bytes_written != buf_size)
		r = OS_IOERR;

	if (fp != stdout)
		if (fclose(fp) && r == OS_OK)
			r = OS_CANTCLOSE;

	return r;
}


void os_log(const char *msg, uint32_t msg_size)
{
	(void)fwrite(msg, 1, msg_size, stderr);
}


int os_is_console(const char *stream_marker, size_t stream_marker_len)
{
	if (stream_marker_len == sizeof(STD_IN_MARK) - 1 &&
	    !memcmp(stream_marker, STD_IN_MARK, stream_marker_len))
		return os_isatty(stdin);

	if (stream_marker_len == sizeof(STD_OUT_MARK) - 1 &&
	    !memcmp(stream_marker, STD_OUT_MARK, stream_marker_len))
		return os_isatty(stdout);

	if (stream_marker_len == sizeof(STD_ERR_MARK) - 1 &&
	    !memcmp(stream_marker, STD_ERR_MARK, stream_marker_len))
		return os_isatty(stderr);

	return 0;
}
