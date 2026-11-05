/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief A simple, single-threaded, linear memory arena (bump allocator)
 *
 * Memory is allocated sequentially from a caller-provided buffer. Freeing is
 * done for a whole region at once, either by reinitialising the arena (from an
 * older state) or by letting a (scratch) arena copy go out of scope.
 *
 * Typical uses are short-lived scratch allocations inside a function, passed by
 * value, and long-lived program-wide allocations, passed by reference.
 *
 * Use arena_zalloc() or the ARENA_NEW / ARENA_NEW_ARRAY macros to obtain
 * zero-initialized memory. Use arena_alloc() if you do not need zeroing.
 *
 * Allocation failures are not reported per call, they are handled through an
 * out-of-memory handler that aborts.
 *
 * @see https://nullprogram.com/blog/2023/09/27/
 */

#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "bithacks.h"

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#  define ARENA_NORETURN [[noreturn]]
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#  define ARENA_NORETURN _Noreturn
#elif defined(__GNUC__) || defined(__clang__)
#  define ARENA_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#  define ARENA_NORETURN __declspec(noreturn)
#else
#  define ARENA_NORETURN
#endif

/** Allocate and zero-initialize a single object of type t */
#define ARENA_NEW(a, t)          ((t *)arena_zalloc(a, 1, sizeof(t), __alignof__(t)))
/** Allocate and zero-initialize an array of n objects of type t */
#define ARENA_NEW_ARRAY(a, n, t) ((t *)arena_zalloc(a, n, sizeof(t), __alignof__(t)))


struct arena {
	uint8_t *beg;
	uint8_t *end;
};


/**
 * @brief Trigger the arena out-of-memory handler
 *
 *  The handler is expected to terminate the program.
 */

ARENA_NORETURN void arena_oom(void);


/**
 * @brief Sets a custom out-of-memory handler for arena allocations
 *
 * @param handler	function to call on out-of-memory conditions, or NULL to
 *			restore the default handler
 *
 * @warning The handler is expected to terminate the program. If it returns,
 *	arena_oom() calls abort().
 */

void arena_set_oom_handler(void (*handler)(void));


/**
 * @brief Create an arena allocator
 *
 * The caller must keep the buffer alive for the lifetime of any allocations
 * made from the arena.
 *
 * @param buf	buffer backing the arena allocations
 * @param size	size of the buf buffer in bytes
 *
 * @returns an arena struct that allocates from the provided buffer
 */

static __inline struct arena arena_init(void *buf, size_t size)
{
	struct arena a = { 0 };

	assert(buf);
	assert(size <= PTRDIFF_MAX);

	a.beg = buf;
	a.end = (uint8_t *)buf + size;

	return a;
}


/**
 * @brief Allocate an **uninitialized** region from an arena
 *
 * @param a	pointer to the arena to allocate from
 * @param count	number of elements
 * @param size	size of each element in bytes
 * @param align	alignment; must be a power of two
 *
 * @returns pointer to the uninitialized allocation; calls arena_oom() on failure
 */

static __inline void *arena_alloc(struct arena *a, ptrdiff_t count, ptrdiff_t size, ptrdiff_t align)
{
	ptrdiff_t padding, available;
	uint8_t *r;

	assert(count >= 0);
	assert(size > 0);
	assert(align > 0 && is_power_of_two((size_t)align));

	padding = (ptrdiff_t)(-(uintptr_t)a->beg & (uintptr_t)(align - 1));
	available = a->end - a->beg - padding;
	if (available < 0 || count > available / size)
		arena_oom();

	r = a->beg + padding;
	a->beg += padding + (count * size);
	return r;
}


/**
 * @brief Allocate a zero-initialized region from an arena
 *
 * @param a	pointer to the arena to allocate from
 * @param count	number of elements
 * @param size	size of each element in bytes
 * @param align	alignment; must be a power of two
 *
 * @returns pointer to the zero-initialized allocation; calls arena_oom() on failure
 */

static __inline void *arena_zalloc(struct arena *a, ptrdiff_t count, ptrdiff_t size,
				   ptrdiff_t align)
{
	void *r = arena_alloc(a, count, size, align);

	memset(r, 0, (size_t)(count * size));
	return r;
}


/**
 * @brief Bytes still available at a given alignment
 *
 * @returns the maximum number of bytes that can still be allocated by an arena
 */

static __inline ptrdiff_t arena_remaining(struct arena a, ptrdiff_t align)
{
	ptrdiff_t padding, available;

	assert(align > 0 && is_power_of_two((size_t)align));

	if (!a.beg || !a.end)
		return 0;

	padding = (ptrdiff_t)(-(uintptr_t)a.beg & (uintptr_t)(align - 1));
	available = a.end - a.beg - padding;
	return available > 0 ? available : 0;
}


/**
 * @brief Check whether buf ends at the current arena top
 *
 * This check does not fully verify that buf was allocated from a, or that size
 * matches the original allocation size.
 *
 * @returns non-zero if buf + size matches the current arena top
 */

static __inline int arena_buf_ends_at_top(struct arena a, const void *buf, ptrdiff_t size)
{
	assert(a.beg && a.end);
	assert(a.beg <= a.end);
	assert(size >= 0);

	/* ptr may not belong to the arena, so (char *)ptr + size can be UB. */
	return buf && (uintptr_t)buf + (uintptr_t)size == (uintptr_t)a.beg;
}


/**
 * @brief Shrink the last arena allocation in place
 *
 * @warning Undefined behaviour if alloc and alloc_size do not describe the
 *	most recent allocation.
 */

static __inline void arena_shrink_last(struct arena *a, const void *alloc MAYBE_UNUSED,
				       ptrdiff_t alloc_size, ptrdiff_t shrunk_size)
{
	assert(alloc_size >= 0);
	assert(shrunk_size <= alloc_size);
	assert(arena_buf_ends_at_top(*a, alloc, alloc_size));

	a->beg = a->beg - alloc_size + shrunk_size;
}


/**
 * @brief Grow the last arena allocation in place
 *
 * Calls the out-of-memory handler if there is not enough remaining space.
 *
 * @warning Undefined behaviour if alloc and alloc_size do not describe the
 *	most recent allocation.
 */

static __inline void arena_grow_last(struct arena *a, const void *alloc MAYBE_UNUSED,
				     ptrdiff_t alloc_size, ptrdiff_t grow_size)
{
	assert(grow_size >= 0);
	assert(grow_size >= alloc_size);
	assert(arena_buf_ends_at_top(*a, alloc, alloc_size));

	(void)ARENA_NEW_ARRAY(a, grow_size - alloc_size, uint8_t);
}

#endif /* ARENA_H */
