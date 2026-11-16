/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief A simple, single-threaded, linear memory arena (bump allocator)
 *
 * Start with:
 * @code
 * struct arena a = arena_init(buf, size);
 * @endcode
 * Keep buf alive while allocations are in use.
 *
 * Use arena_zalloc() or the ARENA_NEW / ARENA_NEW_ARRAY macros to obtain
 * zero-initialized memory. Use arena_alloc() if you do not want zeroing.
 *
 * Running out of space terminates the program by default.
 * Set a custom OOM handler with arena_set_oom_handler(), it must not return.
 *
 * There is no per-object free. Pass an arena by pointer to keep allocations,
 * or by value for scratch space. Use separate buffers for perm and scratch:
 * @code
 * int *foo(struct arena *perm, struct arena scratch)
 * {
 *     int *result = ARENA_NEW(perm, int);
 *     int *tmp = ARENA_NEW(&scratch, int);
 *
 *     *tmp = 42;
 *     *result = *tmp;
 *     return result;
 * }
 * @endcode
 * Only perm advances in the caller. Do not return tmp or keep it elsewhere,
 * later allocations can overwrite it.
 *
 * Debugging with poisoning:
 * With AddressSanitizer (ASan), MemorySanitizer (MSan), or custom hooks, unused
 * gaps between allocations (red zones) are added and poisoned.
 * Red zones help catch accesses past an allocation's end that would otherwise
 * land in the next allocation and go unnoticed.
 * Red zones and minimum alignment padding consume space in the provided buffer,
 * reducing the capacity available for allocations.
 * Note that scratch memory is not automatically poisoned when it goes out of
 * scope and stale pointers may go undetected after their memory is allocated
 * again.
 * Red zone poisoning is enabled automatically when ASan or MSan instrumentation
 * is detected.
 *
 * Options to set before including this header:
 * - ARENA_RED_ZONE_SIZE: gap in bytes defaults to 32 when poisoning is on,
 *   otherwise 0. Set to 0 to disable explicit gaps. This does not disable
 *   poisoning of the tail or alignment padding.
 * - ARENA_MIN_ALIGNMENT: minimum byte alignment; must be a positive power of two.
 *   Defaults to 8 when ARENA_ASAN_ENABLED is 1, otherwise 1.
 *   ASan tracks accessible prefixes of 8-byte groups, so smaller
 *   alignment can leave red-zone bytes accessible.
 *   See https://github.com/mcgov/asan_alignment_example.
 *   Testing with 1 can reveal allocation alignment mistakes hidden by stronger
 *   alignment.
 * - ARENA_ASAN_ENABLED: auto-detected; define as 0 or 1 to override ASan usage.
 * - ARENA_MSAN_ENABLED: auto-detected; define as 0 or 1 to override MSan usage.
 *   Clang supports MSan; GCC currently does not. ASan takes priority over MSan.
 *
 * ARENA_POISON_ENABLED reports whether poisoning is on.
 * Do not define it yourself.
 *
 * For another checker, define both hooks. They override both sanitizer backends.
 * For example, build with this setup and run under valgrind --tool=memcheck:
 * @code
 * #include <valgrind/memcheck.h>
 * #define ARENA_POISON(p, n)   ((void)VALGRIND_MAKE_MEM_NOACCESS(p, n))
 * #define ARENA_UNPOISON(p, n) ((void)VALGRIND_MAKE_MEM_UNDEFINED(p, n))
 * #include "arena.h"
 * @endcode
 * Hooks get non-NULL pointers and nonzero size_t byte counts. Poison marks bytes
 * inaccessible or uninitialized. Unpoison exposes bytes without clearing
 * them. Neither hook may change the contents. Use the same hooks and options in
 * all files sharing arenas.
 *
 * Before clearing or reusing buf outside the arena, finish all arena users
 * and call ARENA_UNPOISON(buf, size) with the original buffer and size.
 * ASan's intercepted free() needs no prior unpoison call.
 *
 * @see https://nullprogram.com/blog/2023/09/27/
 * @see https://clang.llvm.org/docs/MemorySanitizer.html
 * @see https://github.com/llvm/llvm-project/blob/main/compiler-rt/include/sanitizer/asan_interface.h
 * @see https://github.com/llvm/llvm-project/blob/main/compiler-rt/include/sanitizer/msan_interface.h
 */

#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "bithacks.h"
#include "compiler.h"

#ifdef ARENA_POISON_ENABLED
#  error "ARENA_POISON_ENABLED is defined by arena.h"
#endif

#ifndef ARENA_ASAN_ENABLED
#  define ARENA_ASAN_ENABLED CMP_ASAN_ENABLED
#endif

#ifndef ARENA_MSAN_ENABLED
#  define ARENA_MSAN_ENABLED CMP_MSAN_ENABLED
#endif

/* Explicit hooks override automatic backends; ASan takes priority over MSan. */
#if defined(ARENA_POISON) != defined(ARENA_UNPOISON)
#  error "Define ARENA_POISON and ARENA_UNPOISON together"
#elif defined(ARENA_POISON)
#  define ARENA_POISON_ENABLED 1
#elif ARENA_ASAN_ENABLED
#  include <sanitizer/asan_interface.h>
#  define ARENA_ASAN_BACKEND        1
#  define ARENA_POISON_ENABLED      1
#  define ARENA_POISON(ptr, size)   __asan_poison_memory_region(ptr, size)
#  define ARENA_UNPOISON(ptr, size) __asan_unpoison_memory_region(ptr, size)
#elif ARENA_MSAN_ENABLED
#  include <sanitizer/msan_interface.h>
#  define ARENA_MSAN_BACKEND        1
#  define ARENA_POISON_ENABLED      1
/* Unlike legacy __msan_poison(), this also records the allocation origin. */
#  define ARENA_POISON(ptr, size)   __msan_allocated_memory(ptr, size)
/*
 * MSan tracks initialization, not accessibility. __msan_unpoison() would mark
 * unwritten payload bytes as initialized and hide uninitialized uses.
 */
#  define ARENA_UNPOISON(ptr, size) ((void)0)
#else
#  define ARENA_POISON_ENABLED      0
#  define ARENA_POISON(ptr, size)   ((void)0)
#  define ARENA_UNPOISON(ptr, size) ((void)0)
#endif

#ifndef ARENA_MIN_ALIGNMENT
#  if ARENA_ASAN_ENABLED
#    define ARENA_MIN_ALIGNMENT 8
#  else
#    define ARENA_MIN_ALIGNMENT 1
#  endif
#endif
#if ARENA_MIN_ALIGNMENT <= 0 || (ARENA_MIN_ALIGNMENT & (ARENA_MIN_ALIGNMENT - 1)) != 0 || \
	ARENA_MIN_ALIGNMENT > PTRDIFF_MAX
#  error "ARENA_MIN_ALIGNMENT must be a positive power of two <= PTRDIFF_MAX"
#endif
#if defined(ARENA_ASAN_BACKEND) && ARENA_MIN_ALIGNMENT < 8
#  warning "ASan arena alignment below 8 bytes can leave parts of red zones accessible"
#endif

/* Enable red zones with poisoning unless the caller chose a size. */
#ifndef ARENA_RED_ZONE_SIZE
#  if ARENA_POISON_ENABLED
#    define ARENA_RED_ZONE_SIZE 32
#  else
#    define ARENA_RED_ZONE_SIZE 0
#  endif
#endif
#if ARENA_RED_ZONE_SIZE < 0 || ARENA_RED_ZONE_SIZE > PTRDIFF_MAX
#  error "ARENA_RED_ZONE_SIZE must be between zero and PTRDIFF_MAX"
#endif

/** @brief Allocate an object of type t and clear its bytes. */
#define ARENA_NEW(a, t)          ((t *)arena_zalloc(a, 1, sizeof(t), __alignof__(t)))
/** @brief Allocate n objects of type t and clear their bytes. */
#define ARENA_NEW_ARRAY(a, n, t) ((t *)arena_zalloc(a, n, sizeof(t), __alignof__(t)))


struct arena {
	uint8_t *beg;
	uint8_t *end;
};


/** @brief Invoke the OOM handler, it must not return. */
CMP_NORETURN void arena_oom(void);


/**
 * @brief Set a terminating OOM handler.
 *
 * NULL restores the default.
 */

void arena_set_oom_handler(void (*handler)(void));


/* Backend hooks require non-NULL pointers and nonzero sizes. */
static __inline void arena_poison_int(void *ptr, ptrdiff_t size)
{
	assert(size >= 0);
	if (size > 0 && ptr)
		ARENA_POISON(ptr, (size_t)size);
}

static __inline void arena_unpoison_int(void *ptr, ptrdiff_t size)
{
	assert(size >= 0);
	if (size > 0 && ptr)
		ARENA_UNPOISON(ptr, (size_t)size);
}


/**
 * @brief Create an arena allocator
 *
 * The caller must keep the buffer alive for the lifetime of any allocations
 * made from the arena. With poisoning enabled, some capacity is reserved for red
 * zones and end alignment. The owner must unpoison the buffer before ordinary
 * reuse of the backing buffer.
 *
 * @param buf	buffer backing the arena allocations; for portable typed
 *		allocations, use storage with no declared type, such as a
 *		malloc() allocation. Declared buffers such as byte arrays
 *		rely on a GCC/Clang workaround.
 *
 * @param size	size of the buf buffer in bytes
 *
 * @returns an arena struct that allocates from the provided buffer
 */

static __inline struct arena arena_init(void *buf, size_t size)
{
	struct arena a = { 0 };

	assert(buf);
	assert(size <= PTRDIFF_MAX);

#if defined(__GNUC__) || defined(__clang__)
	/*
	 * Hide the buffer pointer's origin from the optimizer.
	 *
	 * A declared byte array is not general-purpose typed storage under
	 * ISO C99 and later: alignment alone does not permit accessing it
	 * through unrelated non-character types.
	 *
	 * This empty asm makes buf appear modified by an opaque operation,
	 * working around optimizations that depend on its known origin.
	 * It does not change buf's C type, alter effective-type rules, or
	 * disable alias analysis for subsequent accesses. This is a
	 * GCC/Clang-specific workaround, not an ISO C guarantee.
	 *
	 * For portable typed storage, use an allocation with no declared
	 * type, such as malloc(), with suitable alignment and initialization.
	 *
	 * Source: https://nullprogram.com/blog/2023/09/27/
	 */
	__asm__("" : "+r"(buf));
#endif

	a.beg = buf;
	a.end = (uint8_t *)buf + size;

	if (ARENA_POISON_ENABLED) {
		ptrdiff_t padding;

		arena_poison_int(buf, (ptrdiff_t)size);

		/*
		 * Align the usable end so ASan can reliably poison unused space.
		 * +------------------+---------------+-------------+
		 * | usable capacity  | alignment pad |  red zone   |
		 * +------------------+---------------+-------------+
		 * ^a.beg    new a.end^
		 */
		padding = align_down_offset((uintptr_t)a.end - (uintptr_t)ARENA_RED_ZONE_SIZE,
					    ARENA_MIN_ALIGNMENT);

		a.end -= min_size(size, (size_t)padding + (size_t)ARENA_RED_ZONE_SIZE);
	}

	return a;
}


/* Internal alignment padding after the leading red zone, excluding the red zone. */
static __inline ptrdiff_t arena_alignment_padding(struct arena a, ptrdiff_t align)
{
	uintptr_t addr;

	assert(align > 0 && is_power_of_two((size_t)align));

	align = max_ptrdiff(align, ARENA_MIN_ALIGNMENT);
	addr = (uintptr_t)a.beg + (uintptr_t)ARENA_RED_ZONE_SIZE;
	return align_up_offset(addr, align);
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
	ptrdiff_t available, padding, bytes;
	uint8_t *alloc;

	assert(count >= 0);
	assert(size > 0);
	assert(align > 0 && is_power_of_two((size_t)align));
	assert(a->beg && a->end && a->beg <= a->end);

	/*
	 * old a->beg             alloc                new a->beg    a->end
	 *     v                    v                       v           v
	 *     +----------+---------+-----------------------+-----------+
	 *     | red zone | padding | payload: count * size | free tail |
	 *     +----------+---------+-----------------------+-----------+
	 */
	padding = arena_alignment_padding(*a, align);
	available = a->end - a->beg - (ptrdiff_t)ARENA_RED_ZONE_SIZE;
	if (available < padding || count > (available - padding) / size)
		arena_oom();

	alloc = a->beg + ARENA_RED_ZONE_SIZE + padding;
	bytes = count * size;

	/*
	 * Scratch allocations remain accessible after their arena copy goes out of scope.
	 * Re-poison the unused tail in one call as it is faster for small allocations.
	 */
	arena_poison_int(a->beg, a->end - a->beg);
	arena_unpoison_int(alloc, bytes);

	a->beg = alloc + bytes;
	return alloc;
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
 * @returns the maximum payload size for the *next* allocation, after accounting
 *	for red zones and alignment
 */

static __inline ptrdiff_t arena_remaining(struct arena a, ptrdiff_t align)
{
	ptrdiff_t available, padding;

	assert(align > 0 && is_power_of_two((size_t)align));
	if (!a.beg || !a.end)
		return 0;
	assert(a.beg <= a.end);

	padding = arena_alignment_padding(a, align);
	available = a.end - a.beg - (ptrdiff_t)ARENA_RED_ZONE_SIZE;
	if (available < padding)
		return 0;

	return available - padding;
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

	/* buf may not belong to the arena, so (char *)buf + size can be UB. */
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
	assert(shrunk_size >= 0 && shrunk_size <= alloc_size);
	assert(arena_buf_ends_at_top(*a, alloc, alloc_size));

	a->beg -= alloc_size - shrunk_size;
	/* Also invalidate any scratch allocations beyond the new top. */
	arena_poison_int(a->beg, a->end - a->beg);
}


/**
 * @brief Grow the last arena allocation in place
 *
 * Existing bytes are preserved. The added tail is uninitialized, not zeroed.
 * Calls the out-of-memory handler if there is not enough remaining space.
 * Capacity is checked before changing annotations or the arena.
 *
 * @warning Undefined behaviour if alloc and alloc_size do not describe the
 *	most recent allocation.
 */

static __inline void arena_grow_last(struct arena *a, const void *alloc MAYBE_UNUSED,
				     ptrdiff_t alloc_size, ptrdiff_t grow_size)
{
	ptrdiff_t extra;

	assert(alloc_size >= 0);
	assert(grow_size >= alloc_size);
	assert(arena_buf_ends_at_top(*a, alloc, alloc_size));

	extra = grow_size - alloc_size;
	if (extra > a->end - a->beg)
		arena_oom();

	/* We poison and unpoison the grown region as unpoison is a no-op for MSan.*/
	arena_poison_int(a->beg, a->end - a->beg);
	arena_unpoison_int(a->beg, extra);
	a->beg += extra;
}

#endif /* ARENA_H */
