/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Collection of some Bit Twiddling Hacks
 *
 * form:
 * @see https://graphics.stanford.edu/~seander/bithacks.html by
 *	@author Sean Eron Anderson et al. and
 * @see linux/tools/include/linux by @author Torvalds et al.
 */

#ifndef BITHACKS_H
#define BITHACKS_H

#include <stdint.h>
#include <limits.h>
#include <stddef.h>

#include "compiler.h"


/** @brief Returns the maximum of two unsigned 32-bit */
static __inline uint32_t max_u32(uint32_t a, uint32_t b)
{
	return a > b ? a : b;
}


/** @brief Returns the minimum of two unsigned 32-bit  */
static __inline uint32_t min_u32(uint32_t a, uint32_t b)
{
	return a < b ? a : b;
}


/** @brief Returns the minimum of two unsigned 64-bit  */
static __inline uint64_t min_u64(uint64_t a, uint64_t b)
{
	return a < b ? a : b;
}


/**
 * @brief Checks whether an unsigned integer is a power of two
 *
 * Zero is not a power of two.
 * @see https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
 *
 * @param n	unsigned integer to check
 * @returns 1 if @p n is a power of two, 0 otherwise
 */
static __inline int is_power_of_two(size_t n)
{
	return n != 0 && (n & (n - 1)) == 0;
}


/**
 * @brief Checks whether a pointer is aligned to a power-of-two boundary
 *
 * An alignment of zero or an alignment that is not a power of two is
 * considered invalid and returns false.
 *
 * @param ptr		pointer to check
 * @param alignment	alignment boundary; must be a power of two
 * @returns 1 if aligned, 0 otherwise
 */
static __inline int is_aligned(const void *ptr, size_t alignment)
{
	uintptr_t mask;

	if (!is_power_of_two(alignment))
		return 0;

	mask = (uintptr_t)alignment - 1;
	return ((uintptr_t)ptr & mask) == 0;
}


/**
 * @brief Checks whether an unsigned integer is a multiple of another
 *
 * This function is equivalent to @c n % multiple == 0, except that it does
 * not evaluate a modulo operation when @p multiple is zero. In that case,
 * zero is a multiple of zero, while any non-zero value is not.
 *
 * @param n		integer to check
 * @param multiple	multiple to check against
 * @returns 1 if @p n is a multiple, 0 otherwise
 *
 * Examples:
 * @code
 * is_multiple_of(6, 2); // 1
 * is_multiple_of(5, 2); // 0
 * is_multiple_of(0, 0); // 1
 * is_multiple_of(6, 0); // 0
 * @endcode
 */
static __inline int is_multiple_of(uint32_t n, uint32_t multiple)
{
	return multiple == 0 ? n == 0 : n % multiple == 0;
}


/**
 * @brief Divides two unsigned 32-bit numbers rounding up
 *
 * The denominator must be non-zero. Unlike (n + d - 1) / d, this cannot
 * overflow before the division.
 */
static __inline uint32_t div_round_up_u32(uint32_t n, uint32_t d)
{
	return (n / d) + (n % d != 0);
}


/** @brief Divides two unsigned 64-bit numbers rounding up */
static __inline uint64_t div_round_up_u64(uint64_t n, uint64_t d)
{
	return (n / d) + (n % d != 0);
}


/** @brief Returns floor(log2(x)) or UINT_MAX if x = 0 */
static __inline unsigned int ilog2(uint32_t x)
{
	compile_time_assert(sizeof(unsigned int) >= sizeof(uint32_t),
			    _expect_unsigned_int_to_be_at_least_32_bit);

	if (x == 0)
		return UINT_MAX;

	return bitsizeof(x) - 1 - (unsigned int)__builtin_clz(x);
}


/** @brief Calculates the floor of division by 2 */
static __inline int32_t floor_division_by_2(int32_t numerator)
{
	compile_time_assert((-1 >> 1) == -1, require_arithmetic_right_shift);
	return numerator >> 1;
}


/** @brief Calculates the floor of division by 4 */
static __inline int32_t floor_division_by_4(int32_t numerator)
{
	compile_time_assert((-1 >> 2) == -1, require_arithmetic_right_shift);
	return numerator >> 2;
}

#endif /*  BITHACKS_H */
