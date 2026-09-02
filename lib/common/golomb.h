/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Common Golomb coding helpers need by compression and decompression
 */

#ifndef CMP_COMMON_GOLOMB_H
#define CMP_COMMON_GOLOMB_H

#include <limits.h>
#include <stdint.h>

#include "../cmp.h"
#include "bithacks.h"
#include "compiler.h"

#define CMP_MAX(a, b) (((a) > (b)) ? (a) : (b))

enum {
	CMP_MIN_GOLOMB_PAR = 1,
	CMP_MAX_GOLOMB_PAR = UINT16_MAX,
	/* The current plan is to only encode uint16_t values. */
	CMP_NUM_BITS_PER_SAMPLE = bitsizeof(uint16_t),
	CMP_GOLOMB_MAX_CODEWORD_BITS = 32
};


/**
 * @brief Calculates the maximum safe outlier threshold
 *
 * @param g_par		Golomb parameter
 * @param n_bits	number of bits used to represent uncompressed samples
 * @param encoder_type	Golomb encoder type used
 *
 * @returns the largest outlier threshold for which all Golomb codewords emitted
 *	by the selected encoder fit within the maximum codeword length, or 0 on
 *	failure
 */

static __inline uint32_t golomb_upper_bound(uint32_t g_par, enum cmp_encoder_type encoder_type,
					    unsigned int n_bits)
{
	uint32_t cutoff, first_invalid_group, first_invalid_value;

	if (g_par < CMP_MIN_GOLOMB_PAR || g_par > CMP_MAX_GOLOMB_PAR)
		return 0;

	if (n_bits > CMP_NUM_BITS_PER_SAMPLE)
		return 0;

	/* How many values live in group 0? */
	cutoff = (2U << ilog2(g_par)) - g_par;

	/*
	 * Determine the minimum group number that would cause a codeword length
	 * overflow.
	 *
	 * The length of a "non 0 group" is calculated as:
	 * len = group_num + 1      // unary_code
	 *     + ilog2(g_par) + 1   // remainder code (fix for a given g_par)
	 *
	 * We want to find the group_num for which len first exceeds
	 * CMP_GOLOMB_MAX_CODEWORD_BITS.
	 * This means we are looking for len to be CMP_GOLOMB_MAX_CODEWORD_BITS + 1.
	 * So: (CMP_GOLOMB_MAX_CODEWORD_BITS + 1) = group_num + ilog2(g_par) + 2
	 * Rearranging for group_num:
	 */
	first_invalid_group = CMP_GOLOMB_MAX_CODEWORD_BITS + 1 - (ilog2(g_par) + 2);

	/*
	 * Convert that group number into the corresponding value.
	 * Each non-zero group contains g_par values.
	 */
	first_invalid_value = cutoff + (first_invalid_group * g_par);

	/* Reserve space for all used multi-escape symbols. */
	if (encoder_type == CMP_ENCODER_GOLOMB_MULTI) {
		uint32_t num_escape_symbols = (n_bits + 1) / 2;

		if (first_invalid_value > num_escape_symbols)
			first_invalid_value -= num_escape_symbols;
		else
			return 0;
	} else if (encoder_type == CMP_ENCODER_GOLOMB_ZERO) {
		/* Zero is reserved as the escape symbol */
		first_invalid_value--;
	}

	return first_invalid_value;
}


/**
 * @brief Calculate an optimal outlier parameter for zero escape mechanism
 *
 * @param g_par		Golomb parameter
 * @param n_bits	number of bits used to represent uncompressed samples
 *
 * @returns the highest optimal outlier value for a given Golomb parameter, when
 *	the zero escape mechanism is used or 0 on fail
 * @warning Value might be too high to encode with golomb_encode().
 *
 * @details We are looking for the smallest v such that:
 *   len_escape < len_golomb(v+1)  (1)  // v+1 so zero can be used as escape symbol
 * where
 *   len_escape = ilog2(g_par)+1        // bits for the zero symbol
 *              + n_bits,               // raw value that follows
 * and
 *   len_golomb(x) = group_num(x) + 1   // unary prefix
 *                 + ilog2(g_par)+1    // binary suffix
 * for x >= cutoff.
 *
 * (Note: If x < cutoff, len_golomb(x) = ilog2(g_par)+1. In this scenario,
 * (1) becomes n_bits < 0, which is impossible.  Thus, the crossover point where
 * escape becomes cheaper must occur when v+1 >= cutoff.)
 *
 * (1) simplifies to:
 *   n_bits < group_num(v+1) + 1,
 * -> n_bits =< group_num(v+1).
 *
 * With
 *   group_num(v) = floor((v − cutoff) / g_par),
 * we get
 *   n_bits =< floor((v+1 - cutoff) / g_par)  (2)
 *
 * The lowest v value that satisfies (2) is therefore the last element of
 * group (n_bits-1):
 *    v_low = cutoff + n_bits * g_par - 1
 *
 * That value becomes our outlier threshold – every mapped sample from
 * that value onwards will be encoded via the zero-escape method.
 */

static __inline uint32_t golomb_optimal_outlier_zero(uint32_t g_par, unsigned int n_bits)
{
	uint32_t cutoff;
	uint64_t outlier;

	if (g_par < CMP_MIN_GOLOMB_PAR || g_par > CMP_MAX_GOLOMB_PAR)
		return 0;

	if (n_bits < 1 || n_bits > CMP_GOLOMB_MAX_CODEWORD_BITS)
		return 0;

	/* size of group 0 */
	cutoff = (2U << ilog2(g_par)) - g_par;

	/*
	 * Calculate last member in group (n_bits-1).
	 * Use 64-bit to prevent overflow when g_par is large.
	 */
	outlier = cutoff + ((uint64_t)n_bits * g_par) - 1;

	/*
	 * Cap at UINT32_MAX since we're returning uint32_t and our encoding
	 * functions can't handle larger values anyway.
	 */
	if (outlier > UINT32_MAX)
		return UINT32_MAX;

	return (uint32_t)outlier;
}

#endif /* CMP_COMMON_GOLOMB_H */
