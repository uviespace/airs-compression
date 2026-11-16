/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Compression Encoder Implementation
 */

#include <stdint.h>
#include <string.h>

#include "encoder.h"
#include "../cmp.h"
#include "../common/bitstream_writer.h"
#include "../common/err_private.h"
#include "../common/bithacks.h"
#include "../common/compiler.h"
#include "../common/golomb.h"

enum {
	CMP_MAX_BITS_ZERO_ESCAPE = 31 - __builtin_clz((uint32_t)CMP_MAX_GOLOMB_PAR) + 1 +
		CMP_NUM_BITS_PER_SAMPLE,
	/* In the worst case, each sample is encoded as an escape (max codeword + raw sample bits) */
	CMP_MAX_BITS_MULTI_ESCAPE = CMP_GOLOMB_MAX_CODEWORD_BITS + CMP_NUM_BITS_PER_SAMPLE,
	CMP_MAX_BITS_PER_SAMPLE = CMP_MAX(CMP_MAX_BITS_ZERO_ESCAPE, CMP_MAX_BITS_MULTI_ESCAPE)
};


uint32_t cmp_encoder_init(struct cmp_encoder *enc, enum cmp_encoder_type encoder_type,
			  uint32_t encoder_param, uint32_t outlier, uint32_t n_bits)
{
	if (!enc)
		return CMP_ERROR(INT_ENCODER);

	memset(enc, 0, sizeof(*enc));
	enc->encoder_type = encoder_type;
	enc->n_bits = n_bits;

	switch (enc->encoder_type) {
	case CMP_ENCODER_UNCOMPRESSED:
		break;

	case CMP_ENCODER_GOLOMB_ZERO:
	case CMP_ENCODER_GOLOMB_MULTI:
		if (encoder_param < CMP_MIN_GOLOMB_PAR || encoder_param > CMP_MAX_GOLOMB_PAR)
			return CMP_ERROR(PARAMS_INVALID);
		enc->g_par = encoder_param;
		enc->g_par_log2 = ilog2(encoder_param);

		if (enc->encoder_type == CMP_ENCODER_GOLOMB_ZERO)
			enc->outlier = golomb_optimal_outlier_zero(enc->g_par, n_bits);
		else
			enc->outlier = outlier;

		/* ensure we do not Golomb-encode too large values */
		enc->outlier = min_u32(enc->outlier,
				       golomb_upper_bound(enc->g_par, enc->encoder_type, n_bits));
		if (enc->outlier == 0)
			return CMP_ERROR(PARAMS_INVALID);
		break;

	default:
		return CMP_ERROR(PARAMS_INVALID);
	}

	return CMP_ERROR(NO_ERROR);
}


uint32_t cmp_encoder_params_check(enum cmp_encoder_type encoder_type, uint32_t encoder_param,
				  uint32_t outlier)
{
	struct cmp_encoder enc_dummy;

	return cmp_encoder_init(&enc_dummy, encoder_type, encoder_param, outlier, 16);
}


/**
 * @brief Sign-extend a value to fill the full width of the integer type
 *
 * @param value		value to sign-extend
 * @param n_bits	number of bits used to represent the value (including sign
 *			bit) in range [0, 32], if 0 returns the value unchained
 *
 * @see https://graphics.stanford.edu/~seander/bithacks.html#VariableSignExtend
 * @returns the sign-extended value
 */

static int32_t sign_extend(int32_t value, unsigned int n_bits)
{
	compile_time_assert((-1 >> 1) == -1, Arithmetic_shift_need);
	unsigned int const extend_bits = (bitsizeof(value) - n_bits) & (bitsizeof(value) - 1);

	return (int32_t)((uint32_t)value << extend_bits) >> extend_bits;
}


/**
 * Map a signed integer to unsigned via ZigZag encoding
 *
 * @param value		signed integer to map
 * @param n_bits	number of bits needed to represent the highest possible
 *			value in range [1, 32]; 0 if treated as 32 bits
 *
 * This function maps negative values to uneven numbers and positive values to
 * even numbers: 0 -> 0, -1 -> 1, 1 -> 2, -2 -> 3, ...  value_MAX -> 2^n_bits - 2,
 *               value_MIN -> 2^n_bits - 1
 *
 * This is needed because Golomb code only works with unsigned values
 * @see https://stackoverflow.com/questions/4533076/google-protocol-buffers-zigzag-encoding
 *
 * @returns a ZigZag encoded unsigned integer
 */

static uint32_t map_to_unsigned(int32_t value, unsigned int n_bits)
{
	compile_time_assert((-1 >> 1) == -1, Arithmetic_shift_need);
	uint32_t const reg_mask = bitsizeof(value) - 1;

	/*
	 * The arithmetic right shift of a negative number (value >> (n_bits - 1))
	 * results in -1 (all bits set), and for a non-negative number, it
	 * results in 0.
	 */
	value = sign_extend(value, n_bits);
	return (((uint32_t)value << 1) ^ (uint32_t)(value >> ((n_bits - 1) & reg_mask)));
}


/**
 * @brief Forms a codeword according to the Golomb code
 *
 * @param value		value to be encoded, must be smaller than
 *			golomb_upper_bound()
 * @param g_par		Golomb parameter (have to be bigger than 0)
 * @param g_par_log2	is ilog2(g_par) calculate outside function for better
 *			performance
 * @param bs		pointer to a bitstream writer; must be initialised by
 *			the caller
 *
 * @warning There is no check of the validity of the input parameters!
 */

static void golomb_encode(uint32_t value, uint32_t g_par, uint32_t g_par_log2,
			  struct bitstream_writer *bs)
{
	uint32_t const cutoff = (2U << g_par_log2) - g_par; /* members in group 0 */

	if (value < cutoff) { /* group 0 */
		bitstream_add_bits32(bs, value, g_par_log2 + 1);
	} else { /* other groups */
		uint32_t const reg_mask = bitsizeof(value) - 1;
		uint32_t const group_num = (value - cutoff) / g_par;
		uint32_t const remainder = value - cutoff - (group_num * g_par);
		uint32_t const unary_code = (1U << (group_num & reg_mask)) - 1;
		uint32_t const base_codeword = cutoff << 1;
		uint32_t len = g_par_log2 + 1;
		uint32_t codeword = unary_code << ((len + 1) & reg_mask);

		codeword += base_codeword + remainder;
		len += 1 + group_num; /* length of the codeword */

		bitstream_add_bits32(bs, codeword, len);
	}
}


void cmp_encoder_encode(const struct cmp_encoder *enc, int16_t value, struct bitstream_writer *bs)
{
	switch (enc->encoder_type) {
	case CMP_ENCODER_UNCOMPRESSED:
		bitstream_add_bits32(bs, (uint16_t)value, enc->n_bits);
		break;

	case CMP_ENCODER_GOLOMB_ZERO: {
		uint16_t const mapped = (uint16_t)map_to_unsigned(value, enc->n_bits);

		if (mapped < enc->outlier) {
			/* add 1 for non-outlier values to make space for 0 as escape symbol */
			golomb_encode((uint32_t)mapped + 1, enc->g_par, enc->g_par_log2, bs);
		} else {
			/* A Golomb codeword of 0 indicates raw (unencoded) mapped data follows.
			 * Combine Golomb(0) and raw data into a single write for efficiency.
			 */
			compile_time_assert(CMP_MAX_BITS_ZERO_ESCAPE <= 32, zero_escape_too_large);
			/* Code word for Golomb(0) contains only zeros. */
			unsigned int const leading_zeros = enc->g_par_log2 + 1;

			bitstream_add_bits32(bs, mapped, leading_zeros + enc->n_bits);
		}
		break;
	}

	case CMP_ENCODER_GOLOMB_MULTI: {
		uint16_t const mapped = (uint16_t)map_to_unsigned(value, enc->n_bits);

		if (mapped < enc->outlier) {
			golomb_encode(mapped, enc->g_par, enc->g_par_log2, bs);
		} else {
			/*
			 * Multi-escape:
			 * 1. Determine the "escape level" based on how many raw
			 *    bits are needed for diff = mapped - outlier.
			 *    level 0: 1-2 raw bits (including 0)
			 *    level 1: 3-4 raw bits
			 *    ...
			 * 2. Golomb-encode the escape_symbol = outlier + escape_level.
			 * 3. Append 'diff' using raw bits
			 */
			uint32_t const diff = mapped - enc->outlier;
			unsigned int const level = diff < 4 ? 0 : ilog2(diff) / 2;

			golomb_encode(enc->outlier + level, enc->g_par, enc->g_par_log2, bs);
			bitstream_add_bits32(bs, diff, (level + 1) * 2);
		}
		break;
	}
	}
}


uint64_t cmp_encoder_max_compressed_size(uint32_t num_samples)
{
	uint64_t const max_bits = (uint64_t)num_samples * CMP_MAX_BITS_PER_SAMPLE;

	return div_round_up_u64(max_bits, 8);
}
