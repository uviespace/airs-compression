/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2026
 * @copyright GPL-2.0
 *
 * @brief Platform abstraction layer
 *
 * Define the interface need from a platform
 */

#ifndef OS_H
#define OS_H

#include <stdint.h>
#include <stddef.h>
#include "arena.h"

/**
 * @brief Default CLI arena capacity.
 * A platform may override this before including os.h.
 */
#ifndef OS_ARENA_CAP
#  define OS_ARENA_CAP ((size_t)1 << 26)
#endif

/** Marker for input redirection from standard input */
#define STD_IN_MARK  "//*-stdin-*//"
/** Marker for output redirection to standard output */
#define STD_OUT_MARK "//*-stdout-*//"
/** Marker for output redirection to standard error */
#define STD_ERR_MARK "//*-stderr-*//"

/** Marker for null output (discarding data) */
#if defined(_WIN32)
#  define NULL_MARK "NUL"
#else
#  define NULL_MARK "/dev/null"
#endif

/** Status codes returned by OS I/O operations */
enum os_status { OS_OK = 0, OS_CANTOPEN, OS_IOERR, OS_CANTCLOSE, OS_TOOBIG };

/** Result of a file-read operation */
struct os_load {
	void *buffer;
	uint32_t size;
	enum os_status status;
};


/**
 * @brief Create an arena allocator
 *
 * Its memory lives until exit. The platform chooses the capacity.
 * Calls arena_oom() on failure.
 */
struct arena os_arena(void);


/**
 * @brief Read in a file or stdin
 *
 * @param path		source file path or STD_IN_MARK to read from stdin
 * @param buf		destination buffer
 * @param buf_size	capacity of buf in bytes
 *
 * @returns .status == OS_OK on success, otherwise error
 */
struct os_load os_read(const char *path, void *buf, uint32_t buf_size);


/**
 * @brief Writes to file or stdout
 *
 * @param path		destination file path or STD_OUT_MARK to write to stdout
 * @param buf		source buffer
 * @param buf_size	number of bytes to write
 *
 * @returns OS_OK on success, otherwise error
 */
enum os_status os_save_from_buffer(const char *path, const void *buf, uint32_t buf_size);


/**
 * @brief Make a directory
 *
 * @param path	directory path to create
 *
 * @returns 0 on success or if the directory already exists, otherwise error
 */
int os_make_directory(const char *path);


/**  Writes a log message */
void os_log(const char *msg, uint32_t msg_size);


/** Returns non-zero if the stream is connected to a terminal, 0 otherwise */
int os_is_console(const char *stream_marker, size_t stream_marker_len);

/** Returns non-zero if path is a directory, 0 otherwise  */
int os_is_directory(const char *path);

/** Returns non-zero if path is an empty directory, 0 otherwise */
int os_directory_is_empty(const char *path);

/** Returns non-zero if path is a regular file, 0 otherwise */
int os_is_regular_file(const char *path);

/** Removes a file or an empty directory */
int os_remove(const char *path);

#endif /* OS_H */
