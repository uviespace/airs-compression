/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Common parts of the preprocessing used for compression and decompression
 */

#ifndef CMP_COMMON_PREPROCESSING_H
#define CMP_COMMON_PREPROCESSING_H

#include <stdint.h>

#include "../cmp.h"
#include "bithacks.h"
#include "compiler.h"


/** Maximum allowed model adaptation rate parameter  */
enum { CMP_MAX_MODEL_RATE = 16 };


/**
 * @brief Returns the number of significant bits produced by preprocessing
 *
 * @param type		preprocessing method
 * @param dtype		sample data type
 *
 * @returns the number of bits required by the preprocessed values
 */
static __inline unsigned int preprocessing_get_output_bits(enum cmp_preprocessing type,
							   enum cmp_type dtype)
{
	switch (type) {
	case CMP_PREPROCESS_IWT:
		/* The 16-bit IWT can expand 12-bit input up to 16-bit coefficients. */
		return bitsizeof(int16_t);
	case CMP_PREPROCESS_NONE:
	case CMP_PREPROCESS_DIFF:
	case CMP_PREPROCESS_MODEL:
	default:
		/*
		 * DIFF and MODEL residuals wrap at the source bit depth. For a
		 * 12-bit sample, a difference of 4095 is the same as -1, so the
		 * residual fits in 12 bits rather than requiring an extra bit.
		 */
		if (dtype == CMP_RAW12)
			return 12;
		else
			return bitsizeof(int16_t);
	}
}


/**
 * @brief Calculates the number of bytes required for a model buffer.
 *
 * @param num_samples	number of samples requiring a model state
 * @returns required model buffer size in bytes
 */
static __inline uint32_t model_buffer_size(uint32_t num_samples)
{
	/* Store one 16-bit value per sample for every input format. */
	return num_samples * sizeof(int16_t);
}


static __inline enum cmp_type get_state_sample_type(enum cmp_type state_type)
{
	switch (state_type) {
	case CMP_U16:
	case CMP_RAW12:
		return CMP_U16;
	case CMP_I16:
	case CMP_I16_IN_I32:
	default:
		return CMP_I16;
	}
}


static __inline int16_t update_model_16(int32_t data, int32_t model, int model_rate)
{
#define MODEL_SHIFT_BITS 4
	compile_time_assert(CMP_MAX_MODEL_RATE == 1 << MODEL_SHIFT_BITS,
			    _CMP_MAX_MODEL_RATE_MODEL_SHIFT_BITS_mismatch);
	return (int16_t)((model * model_rate + data * (CMP_MAX_MODEL_RATE - model_rate)) >>
			 MODEL_SHIFT_BITS);
}


static __inline int16_t update_model(int16_t data, int16_t model, int model_rate,
				     enum cmp_type dtype)
{
	switch (dtype) {
	case CMP_I16:
	case CMP_I16_IN_I32:
		return update_model_16(data, model, model_rate);
	case CMP_U16:
	case CMP_RAW12:
	default:
		return update_model_16((uint16_t)data, (uint16_t)model, model_rate);
	}
}

#endif /* CMP_COMMON_PREPROCESSING_H */
