/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Platform-independent file I/O implementation
 */

#include <stdint.h>
#include <assert.h>

#include "file.h"
#include "log.h"
#include "str_slice.h"
#include "../lib/common/byteorder.h"
#include "../lib/common/bithacks.h"
#include "../lib/common/arena.h"

static int g_force_stdin_console;
static int g_force_stdout_console;


void file_force_stdin_console(void)
{
	g_force_stdin_console = 1;
}

void file_force_stdout_console(void)
{
	g_force_stdout_console = 1;
}

int file_is_console(struct s8 stream_mark)
{
	if (g_force_stdin_console && s8_equals(stream_mark, STD_IN_MARK_S8))
		return 1;
	if (g_force_stdout_console && s8_equals(stream_mark, STD_OUT_MARK_S8))
		return 1;
	return os_is_console((const char *)stream_mark.s, (size_t)stream_mark.len);
}


struct os_load file_read(struct arena *perm, struct s8 path, enum file_flags flags)
{
	struct os_load r;
	const char *path_as_cstr = s8_to_cstr(perm, path);
	uint64_t const remaining = (uint64_t)arena_remaining(*perm, __alignof__(uint32_t));
	ptrdiff_t const cap = (ptrdiff_t)min_u64(remaining, UINT32_MAX);
	void *p = arena_alloc(perm, cap, 1, __alignof__(uint32_t));

	assert((flags & ~(unsigned int)FILE_MISSING_OK) == 0 && "invalid flags");

	r = os_read(path_as_cstr, p, (uint32_t)cap);
	if (r.status == OS_OK)
		arena_shrink_last(perm, p, cap, (ptrdiff_t)r.size);
	else
		arena_shrink_last(perm, p, cap, 0);

	switch (r.status) {
	case OS_OK: {
		LOG_DEBUG("Successful read in '%.*s'", (int)path.len, path.s);
		if (r.size == 0) {
			r.status = OS_IOERR;
			LOG_ERROR("'%.*s' is empty.", (int)path.len, path.s);
		}
		break;
	}
	case OS_CANTOPEN:
		if (flags & FILE_MISSING_OK)
			LOG_DEBUG("Can't open '%.*s'", (int)path.len, path.s);
		else
			LOG_ERROR_WITH_ERRNO("Can't open '%.*s'", (int)path.len, path.s);
		break;
	case OS_IOERR:
		LOG_ERROR_WITH_ERRNO("Can't read '%.*s'", (int)path.len, path.s);
		break;
	case OS_CANTCLOSE:
		LOG_ERROR_WITH_ERRNO("Can't close '%.*s'", (int)path.len, path.s);
		break;
	case OS_TOOBIG:
		LOG_ERROR("'%.*s' is too large", (int)path.len, path.s);
		break;
	default:
		assert(0);
	}

	return r;
}


struct os_load file_read_be16(struct arena *perm, struct s8 path, enum file_flags flags)
{
	size_t i;
	struct os_load r;

	r = file_read(perm, path, flags);
	if (r.status != OS_OK)
		return r;

	if (!is_multiple_of((uint32_t)r.size, 2)) {
		LOG_ERROR("%.*s: file size not a multiple of 2", (int)path.len, path.s);
		r.status = OS_IOERR;
		return r;
	}

	for (i = 0; i < r.size / 2; i++)
		be16_to_cpus((uint16_t *)r.buffer + i);

	return r;
}


int file_write(struct arena scratch, struct s8 path, const void *buf, uint32_t buf_size,
	       enum file_flags flags)
{
	int r;
	const char *path_cstr = s8_to_cstr(&scratch, path);

	assert(buf);
	assert((flags & ~(unsigned int)FILE_OVERWRITE) == 0 && "invalid flags");

	if (s8_equals(path, s8_from_cstr(NULL_MARK)))
		return 0;

	if (os_is_directory(path_cstr)) {
		LOG_ERROR("'%.*s' is a directory", (int)path.len, path.s);
		return -1;
	}

	if (os_is_regular_file(path_cstr)) {
		if (flags & FILE_OVERWRITE) {
			LOG_DEBUG("Try to overwrite '%.*s'", (int)path.len, path.s);
		} else {
			LOG_ERROR("'%.*s' already exists", (int)path.len, path.s);
			return -1;
		}
	}

	switch (os_save_from_buffer(path_cstr, buf, buf_size)) {
	case OS_OK:
		LOG_DEBUG("Successful write '%.*s'", (int)path.len, path.s);
		r = 0;
		break;
	case OS_CANTOPEN:
		LOG_ERROR("Can't create '%.*s'", (int)path.len, path.s);
		r = -1;
		break;
	case OS_IOERR:
		LOG_ERROR_WITH_ERRNO("Error writing '%.*s':", (int)path.len, path.s);
		r = -1;
		break;
	case OS_CANTCLOSE:
		LOG_WARNING("File '%.*s' saved successfully but close failed", (int)path.len,
			    path.s);
		r = 0;
		break;
	case OS_TOOBIG:
	default:
		assert(0);
		r = -1;
	}

	return r;
}


int file_save_be16(struct arena scratch, struct s8 path, const uint16_t *buf, uint32_t buf_size,
		   enum file_flags flags)
{
	ptrdiff_t const samples = (ptrdiff_t)(buf_size / sizeof(*buf));
	uint16_t *tmp = ARENA_NEW_ARRAY(&scratch, samples, uint16_t);
	ptrdiff_t i;

	assert(buf);
	assert(buf_size);
	assert(is_multiple_of(buf_size, sizeof(*buf)));

	for (i = 0; i < samples; i++)
		tmp[i] = cpu_to_be16(buf[i]);

	return file_write(scratch, path, tmp, buf_size, flags);
}


int file_make_directory(struct arena scratch, struct s8 path)
{
	const char *path_as_cstr = s8_to_cstr(&scratch, path);
	int r;

	assert(!s8_equals(path, STD_IN_MARK_S8));
	assert(!s8_equals(path, STD_OUT_MARK_S8));
	assert(!s8_equals(path, STD_ERR_MARK_S8));
	assert(!s8_equals(path, NULL_MARK_S8));

	r = os_make_directory(path_as_cstr);
	if (r != 0)
		LOG_ERROR_WITH_ERRNO("Can't create directory '%.*s'", (int)path.len, path.s);

	return r;
}


int file_directory_is_empty(struct arena scratch, struct s8 path)
{
	const char *path_as_cstr = s8_to_cstr(&scratch, path);

	assert(!s8_equals(path, STD_IN_MARK_S8));
	assert(!s8_equals(path, STD_OUT_MARK_S8));
	assert(!s8_equals(path, STD_ERR_MARK_S8));
	assert(!s8_equals(path, NULL_MARK_S8));

	return os_directory_is_empty(path_as_cstr);
}


int file_remove(struct arena scratch, struct s8 path)
{
	const char *path_as_cstr = s8_to_cstr(&scratch, path);
	int r;

	assert(!s8_equals(path, STD_IN_MARK_S8));
	assert(!s8_equals(path, STD_OUT_MARK_S8));
	assert(!s8_equals(path, STD_ERR_MARK_S8));
	assert(!s8_equals(path, NULL_MARK_S8));

	r = os_remove(path_as_cstr);
	if (r != 0)
		LOG_ERROR_WITH_ERRNO("Can't remove '%.*s'", (int)path.len, path.s);

	return r;
}
