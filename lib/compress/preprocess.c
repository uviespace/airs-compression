/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data preprocessing implementation for compression.
 *
 * This file contains functions for various data preprocessing methods
 * used in a compression algorithm.
 * It uses an array of preprocessing_method structures to organize different
 * preprocessing techniques. Each structure includes function pointers for
 * calculating work buffer size, initialise the processing, and processing the
 * data.
 *
 */

#include "common/sample_reader.h"
#include <stdint.h>
#include <stddef.h>

#include "preprocess.h"
#include "../cmp.h"
#include "../common/compiler.h"
#include "../common/err_private.h"


/* ====== Helper Functions for Integer Wavelet Transform (IWT) ===== */
/**
 * @brief Calculates the floor of division by 2
 *
 * @param dividend	the value to divide
 *
 * @returns the result of the division
 */

static __inline int16_t floor_division_by_2(int32_t dividend)
{
	return (int16_t)(dividend >> 1);
}


/**
 * @brief Calculates the floor of division by 4
 *
 * @param dividend	the value to divide
 *
 * @returns the result of the division
 */

static __inline int16_t floor_division_by_4(int32_t dividend)
{
	return (int16_t)(dividend >> 2);
}


/**
 * @brief Calculates the odd (low frequency) transform coefficient of the IWT
 *
 * @param centre	centre value of the kernel
 * @param left		left neighbour value
 * @param right		right neighbour value
 *
 * @returns the odd (low frequency) coefficient
 */

static __inline int16_t iwt_odd_coefficient(int16_t centre, int16_t left, int16_t right)
{
	return centre - floor_division_by_2(left + right);
}


/**
 * @brief Calculates the last odd (low frequency) transform coefficient of the IWT
 *
 * @param centre	centre value of the kernel
 * @param left		left neighbour value
 *
 * @returns the last odd (low frequency) coefficient
 */

static __inline int16_t iwt_last_odd_coefficient(int16_t centre, int16_t left)
{
	return centre - left;
}


/**
 * @brief Calculates the even (high frequency) transform coefficient of the IWT
 *
 * @param centre		centre value of the kernel
 * @param odd_coef_left		left neighbour odd (low frequency) coefficient
 * @param odd_coef_right	right neighbour odd (low frequency) coefficient
 *
 * @returns the even (high frequency) coefficient
 */

static __inline int16_t iwt_even_coefficient(int16_t centre, int16_t odd_coef_left,
					     int16_t odd_coef_right)
{
	return centre + floor_division_by_4(odd_coef_left + odd_coef_right);
}


/**
 * @brief Calculates an edge even (high frequency) transform coefficient of the IWT
 *
 * @param centre		centre value of the kernel
 * @param odd_coef_neighbour	neighbouring odd (low frequency) coefficient
 *
 * @returns the calculated edge even (high frequency) coefficient
 */

static __inline int16_t iwt_edge_even_coefficient(int16_t centre, int16_t odd_coef_neighbour)
{
	return centre + floor_division_by_2(odd_coef_neighbour);
}


/* ====== Integer Wavelet Transform (IWT) Processing ====== */
/**
 * @brief Perform single level integer wavelet transform (IWT) for int16_t data
 *
 * @param x	pointer to the input data
 * @param y	pointer to the output coefficients buffer (can be the same as x
 *		for in-place calculation)
 * @param n	total number of int16_t samples in input and output buffer
 * @param s	stride; spacing between elements processed; must be > 0
 *		(starts at 1, doubles each level in multi-level decomposition)
 *
 * @see implementation is based on equation (5.24) from
 *	D. Solomon, Data Compression, 4th ed, 2007, Springer, pp. 609-607
 *
 * Coefficient Arrangement:
 * - approximation (high frequency) coefficient are stored on odd indexes
 * - detail (low frequency) coefficient are stored on even indexes
 *
 */

static void iwt_single_level_i16(const int16_t *x, int16_t *y, size_t n, size_t s)
{
	size_t i;

	if (n == 0)
		return;

	/* Only one element: the output equals the input */
	if (s >= n) {
		y[0] = x[0];
		return;
	}

	/* Two elements to process, handle as a special case */
	if (2 * s >= n) {
		y[s] = iwt_last_odd_coefficient(x[s], x[0]);
		y[0] = iwt_edge_even_coefficient(x[0], y[s]);
		return;
	}

	/* Compute the first two coefficients outside the loop for performance */
	y[s] = iwt_odd_coefficient(x[s], x[0], x[2 * s]);
	y[0] = iwt_edge_even_coefficient(x[0], y[s]);

	/* Process the coefficients in the middle */
	for (i = 2 * s; i < n - 2 * s; i += 2 * s) {
		y[i + s] = iwt_odd_coefficient(x[i + s], x[i], x[i + 2 * s]);
		y[i] = iwt_even_coefficient(x[i], y[i - s], y[i + s]);
	}

	/* Compute the last coefficient(s) outside the loop for performance */
	if (i < n - s) { /* two elements over? */
		y[i + s] = iwt_last_odd_coefficient(x[i + s], x[i]);
		y[i] = iwt_even_coefficient(x[i], y[i - s], y[i + s]);
	} else {
		y[i] = iwt_edge_even_coefficient(x[i], y[i - s]);
	}
}


/**
 * @brief Performs a multi level integer wavelet transform (IWT) decomposition
 *	on int16_t data
 *
 * @param src_desc	source data descriptor pointer
 * @param output	output buffer for decomposition coefficients (has to be
 *			same size as the input)
 * @param num_samples	number of int16_t samples in the input data buffer
 */

static void iwt_multi_level_decomposition_i16(const struct sample_desc *src_desc, int16_t *output,
					      size_t num_samples)
{
	const int16_t *input;
	size_t stride;

	if (num_samples == 1) {
		output[0] = sample_read_i16(src_desc, 0);
		return;
	}

	if (src_desc->dtype == CMP_I16_IN_I32) {
		uint32_t i;
		/*
		 * For non-contiguous 16-bit samples stored in 32-bit words,
		 * we need to pack them into a contiguous array first.
		 * TODO: Optimize by adding a stride parameter to
		 * iwt_single_level_i16() to process non-contiguous data
		 * directly.
		 */
		for (i = 0; i < num_samples; i++)
			output[i] = sample_read_i16(src_desc, i);
		input = output;
	} else {
		input = src_desc->data;
	}

	for (stride = 1; stride < num_samples; stride <<= 1) {
		iwt_single_level_i16(input, output, num_samples, stride);
		input = output;
	}
}


/* ====== Preprocessing Method Functions ====== */
/**
 * @brief Calculates the required work buffer size for none preprocessing
 *
 * @param input_size	unused
 *
 * @returns 0
 */

static uint32_t none_get_work_buf_size(uint32_t input_size UNUSED)
{
	return 0;
}


/**
 * @brief Initializes none preprocessing
 *
 * @param src_desc	source data descriptor pointer
 * @param work_buf	unused
 * @param work_buf_size	unused
 *
 * @returns returns the number of elements to preprocess or an error, which can
 *	be checked with cmp_is_error()
 */

static uint32_t none_init(const struct sample_desc *src_desc, void *work_buf UNUSED,
			  uint32_t work_buf_size UNUSED)
{
	return src_desc->num_samples;
}


/**
 * @brief Processes data with none preprocessing
 *
 * @param i		index of the data
 * @param src_desc	source data descriptor pointer
 *
 * @param work_buf	unused
 *
 * @returns the processed data at index i
 */

static int16_t none_process(uint32_t i, const struct sample_desc *src_desc, void *work_buf UNUSED)
{
	return sample_read_i16(src_desc, i);
}


/**
 * @brief Processes data using 1d difference preprocessing
 *
 * @param i		index of the data
 * @param src_desc	source data descriptor pointer
 * @param work_buf	unused
 *
 * @returns the processed data at index i
 */

static int16_t diff_process(uint32_t i, const struct sample_desc *src_desc, void *work_buf UNUSED)
{
	if (i == 0)
		return sample_read_i16(src_desc, i);
	else
		return (int16_t)(sample_read_i16(src_desc, i) - sample_read_i16(src_desc, i - 1));
}


/**
 * @brief Calculates the required work buffer size for IWT preprocessing
 *
 * @param input_size	size of the data to perform the IWT on
 *
 * @returns the minimum required work buffer size
 */

static uint32_t iwt_get_work_buf_size(uint32_t input_size)
{
	return ROUND_UP_TO_NEXT_2(input_size);
}


/**
 * @brief Initializes multi level IWT preprocessing
 *
 * This function pre-calculates the IWT coefficient and put them in the working
 * buffer
 *
 * @param src_desc	source data descriptor pointer
 * @param work_buf	pointer to the working buffer for temporary results
 * @param work_buf_size	size in bytes of the working buffer
 *
 * @returns the number of samples to process or an error code is returned (which
 *	can be checked using cmp_is_error()).
 */

static uint32_t iwt_init(const struct sample_desc *src_desc, void *work_buf, uint32_t work_buf_size)
{
	int16_t *pre_cal_coefficient = (int16_t *)work_buf;

	if (!work_buf)
		return CMP_ERROR(WORK_BUF_NULL);
	if (work_buf_size < iwt_get_work_buf_size(get_packed_size(src_desc)))
		return CMP_ERROR(WORK_BUF_TOO_SMALL);
	if ((uintptr_t)work_buf & (sizeof(*pre_cal_coefficient) - 1))
		return CMP_ERROR(WORK_BUF_UNALIGNED);

	iwt_multi_level_decomposition_i16(src_desc, pre_cal_coefficient, src_desc->num_samples);

	return src_desc->num_samples;
}


/**
 * @brief Processes data using multi level IWT preprocessing
 *
 * @param i		index of the data
 * @param src_desc	unused (IWT coefficients are pre-calculated in work_buf)
 * @param work_buf	pointer to the working buffer
 *
 * @returns the processed data at index i
 */

static int16_t iwt_process(uint32_t i, const struct sample_desc *src_desc UNUSED, void *work_buf)
{
	int16_t *pre_cal_coefficient = work_buf;

	return pre_cal_coefficient[i];
}


/**
 * @brief Calculates the required work buffer size for model preprocessing
 *
 * @param input_size	size of the data to perform the preprocessing
 *
 * @returns the minimum required work buffer size
 */

static uint32_t model_get_work_buf_size(uint32_t input_size)
{
	return ROUND_UP_TO_NEXT_2(input_size);
}


/**
 * @brief Initializes model preprocessing
 *
 * @param src_desc	source data descriptor pointer
 * @param work_buf	pointer to the buffer where the model to be subtracted
 *			from data is stored
 * @param work_buf_size	size in bytes of the working buffer
 *
 * @returns the number of samples to process or an error code is returned (which
 *	can be checked using cmp_is_error()).
 */

static uint32_t model_init(const struct sample_desc *src_desc, void *work_buf,
			   uint32_t work_buf_size)
{
	if (!work_buf)
		return CMP_ERROR(WORK_BUF_NULL);
	if (work_buf_size < model_get_work_buf_size(get_packed_size(src_desc)))
		return CMP_ERROR(WORK_BUF_TOO_SMALL);
	if ((uintptr_t)work_buf & (sizeof(uint16_t) - 1))
		return CMP_ERROR(WORK_BUF_UNALIGNED);

	return src_desc->num_samples;
}


/**
 * @brief Processes data using model preprocessing
 *
 * @param i		index of the data
 * @param src_desc	source data descriptor pointer
 * @param work_buf	pointer to the working buffer containing the model
 *
 * @returns the processed data for the i-th data sample
 */

static int16_t model_process(uint32_t i, const struct sample_desc *src_desc, void *work_buf)
{
	const uint16_t *model = work_buf;

	return (int16_t)(sample_read_i16(src_desc, i) - model[i]);
}


/* ====== Public API ====== */
const struct preprocessing_method *preprocessing_get_method(enum cmp_preprocessing type)
{
	static const struct preprocessing_method preprocessing_methods[] = {
		{ CMP_PREPROCESS_NONE,  none_get_work_buf_size,  none_init,  none_process  },
		{ CMP_PREPROCESS_DIFF,  none_get_work_buf_size,  none_init,  diff_process  },
		{ CMP_PREPROCESS_IWT,   iwt_get_work_buf_size,   iwt_init,   iwt_process   },
		{ CMP_PREPROCESS_MODEL, model_get_work_buf_size, model_init, model_process }
	};
	size_t i;

	for (i = 0; i < ARRAY_SIZE(preprocessing_methods); i++) {
		if (preprocessing_methods[i].type == type)
			return &preprocessing_methods[i];
	}
	return NULL;
}
