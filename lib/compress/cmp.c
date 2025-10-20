/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data compression implementation
 */

#include <stdint.h>
#include <string.h>

#include "preprocess.h"
#include "encoder.h"
#include "../cmp.h"
#include "../common/err_private.h"
#include "../common/bitstream_writer.h"
#include "../common/header_private.h"
#include "../common/bithacks.h"
#include "../common/compiler.h"

#define CMP_MAGIC 34021395 /* arbitrary magic number I like */


/* Fallback monotonic counter implementation for g_get_timestamp() */
static void fallback_get_timestamp(uint32_t *coarse, uint16_t *fine)
{
	static uint64_t cnt;

	*coarse = (uint32_t)(cnt >> 16);
	*fine = (uint16_t)cnt;
	cnt++;
}


/**
 * Function pointer to a function providing a timestamp initialised with
 * cmp_set_timestamp_func()
 */
static void (*g_get_timestamp)(uint32_t *coarse, uint16_t *fine) = fallback_get_timestamp;


void cmp_set_timestamp_func(void (*get_current_timestamp_func)(uint32_t *coarse, uint16_t *fine))
{
	if (get_current_timestamp_func)
		g_get_timestamp = get_current_timestamp_func;
	else
		g_get_timestamp = fallback_get_timestamp;
}


unsigned int cmp_is_error(uint32_t code)
{
	return cmp_is_error_int(code);
}


uint32_t cmp_compress_bound(uint32_t src_size)
{
	compile_time_assert(CMP_HDR_MAX_COMPRESSED_SIZE <= UINT32_MAX,
			    max_compressed_bound_exceeds_uint32_max);
	uint64_t bound;

	if (src_size > CMP_HDR_MAX_ORIGINAL_SIZE)
		return (CMP_ERROR(HDR_ORIGINAL_TOO_LARGE));

	bound = CMP_HDR_MAX_SIZE + CMP_CHECKSUM_SIZE + cmp_encoder_max_compressed_size(src_size);

	if (bound > CMP_HDR_MAX_COMPRESSED_SIZE)
		return (CMP_ERROR(HDR_CMP_SIZE_TOO_LARGE));

	return (uint32_t)bound;
}


uint32_t cmp_cal_work_buf_size(const struct cmp_params *params, uint32_t src_size)
{
	const struct preprocessing_method *preprocess;
	uint32_t primary_work_buf_size, secondary_work_buf_size;

	if (params == NULL)
		return CMP_ERROR(GENERIC);

	if (params->primary_preprocessing == CMP_PREPROCESS_MODEL)
		return CMP_ERROR(PARAMS_INVALID);

	preprocess = preprocessing_get_method(params->primary_preprocessing);
	if (preprocess == NULL)
		return CMP_ERROR(PARAMS_INVALID);
	primary_work_buf_size = preprocess->get_work_buf_size(src_size);

	if (params->secondary_iterations) {
		preprocess = preprocessing_get_method(params->secondary_preprocessing);
		if (preprocess == NULL)
			return CMP_ERROR(PARAMS_INVALID);
		secondary_work_buf_size = preprocess->get_work_buf_size(src_size);
	} else {
		secondary_work_buf_size = 0;
	}

	return max_u32(primary_work_buf_size, secondary_work_buf_size);
}


/** Maximum allowed model adaptation rate parameter  */
#define CMP_MAX_MODEL_RATE 16U

/**
 * @brief Updates the model value based on new data and adaptation rate
 *
 * @param data		new data value to incorporate into the model
 * @param model		current model value
 * @param model_rate	model adaptation rate; higher values make the model adapt
 *			more slowly to new data; must be less than or equal to
 *			CMP_MAX_MODEL_RATE
 * @returns the updated model value
 */

static uint16_t update_model(uint16_t data, uint16_t model, unsigned int model_rate)
{
	uint32_t const weighted_data = data * (CMP_MAX_MODEL_RATE - model_rate);
	uint32_t const weighted_model = model * model_rate;

	return (uint16_t)((weighted_model + weighted_data) / CMP_MAX_MODEL_RATE);
}


uint32_t cmp_initialise(struct cmp_context *ctx, const struct cmp_params *params, void *work_buf,
			uint32_t work_buf_size)
{
	uint32_t const min_src_size = 2;
	uint32_t work_buf_needed;
	uint32_t error_code;

	if (ctx == NULL)
		return CMP_ERROR(GENERIC);
	cmp_deinitialise(ctx);

	if (params == NULL)
		return CMP_ERROR(GENERIC);

	if (cmp_is_error_int(work_buf_size))
		return CMP_ERROR(GENERIC);

	if (params->secondary_iterations >= (1ULL << CMP_HDR_BITS_SEQUENCE_NUMBER))
		return CMP_ERROR(PARAMS_INVALID);

	error_code = cmp_encoder_params_check(params->primary_encoder_type,
					      params->primary_encoder_param,
					      params->primary_encoder_outlier);
	if (cmp_is_error_int(error_code))
		return error_code;

	if (params->secondary_iterations) {
		error_code = cmp_encoder_params_check(params->secondary_encoder_type,
						      params->secondary_encoder_param,
						      params->secondary_encoder_outlier);
		if (cmp_is_error_int(error_code))
			return error_code;

		if (params->model_rate > CMP_MAX_MODEL_RATE &&
		    params->secondary_preprocessing == CMP_PREPROCESS_MODEL)
			return CMP_ERROR(PARAMS_INVALID);
	}

	work_buf_needed = cmp_cal_work_buf_size(params, min_src_size);
	if (cmp_is_error_int(work_buf_needed))
		return work_buf_needed;

	if (work_buf_needed) {
		if (!work_buf)
			return CMP_ERROR(WORK_BUF_NULL);
		if (work_buf_size == 0)
			return CMP_ERROR(WORK_BUF_TOO_SMALL);

		if ((uintptr_t)work_buf & (sizeof(uint16_t) - 1))
			return CMP_ERROR(WORK_BUF_UNALIGNED);
	}

	ctx->params = *params;
	ctx->work_buf = work_buf;
	ctx->work_buf_size = work_buf_size;
	ctx->magic = CMP_MAGIC; /* add some magic */

	return cmp_reset(ctx);
}


/* Main compression loop */
static uint32_t compress_u16_engine(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
				    const uint16_t *src, uint32_t src_size)
{
	uint32_t i, ret, n_values;
	enum cmp_preprocessing selected_preprocessing;
	enum cmp_encoder_type selected_encoder_type;
	uint32_t selected_encoder_param;
	uint32_t selected_outlier;
	struct bitstream_writer bs;
	struct cmp_encoder enc;
	const struct preprocessing_method *preprocess;
	int model_is_needed = 0;
	struct cmp_hdr hdr = { 0 };
	uint32_t compress_bound;

	if (ctx->sequence_number == 0 || ctx->sequence_number > ctx->params.secondary_iterations) {
		ret = cmp_reset(ctx);
		if (cmp_is_error_int(ret))
			return ret;
		selected_preprocessing = ctx->params.primary_preprocessing;
		selected_encoder_type = ctx->params.primary_encoder_type;
		selected_encoder_param = ctx->params.primary_encoder_param;
		selected_outlier = ctx->params.primary_encoder_outlier;
		ctx->model_size = src_size;
	} else {
		selected_preprocessing = ctx->params.secondary_preprocessing;
		selected_encoder_type = ctx->params.secondary_encoder_type;
		selected_encoder_param = ctx->params.secondary_encoder_param;
		selected_outlier = ctx->params.secondary_encoder_outlier;
		/*
		 * When using model preprocessing the size of the data to
		 * compression is not allowed to change unit a reset.
		 */
		if (ctx->params.secondary_preprocessing == CMP_PREPROCESS_MODEL &&
		    src_size != ctx->model_size)
			return CMP_ERROR(SRC_SIZE_MISMATCH);
	}

	/* Do we need a model? */
	if (ctx->params.secondary_preprocessing == CMP_PREPROCESS_MODEL &&
	    ctx->params.secondary_iterations != 0) {
		model_is_needed = 1;

		if (ctx->work_buf_size < src_size)
			return CMP_ERROR(WORK_BUF_TOO_SMALL);
	}

	ret = bitstream_writer_init(&bs, dst, dst_capacity);
	if (cmp_is_error_int(ret))
		return ret;

	ret = cmp_encoder_init(&enc, selected_encoder_type, selected_encoder_param,
			       selected_outlier);
	if (cmp_is_error_int(ret))
		return ret;

	hdr.version_flag = 1;
	hdr.version_id = CMP_VERSION_NUMBER;
	hdr.original_size = src_size;
	hdr.compressed_size = 0; /* place holder, not know right now */
	hdr.identifier = ctx->identifier;
	hdr.sequence_number = ctx->sequence_number;
	hdr.preprocessing = selected_preprocessing;
	hdr.checksum_enabled = !!ctx->params.checksum_enabled;
	hdr.encoder_type = selected_encoder_type;
	if (selected_preprocessing == CMP_PREPROCESS_MODEL)
		hdr.model_rate = ctx->params.model_rate;
	if (selected_encoder_type != CMP_ENCODER_UNCOMPRESSED) {
		hdr.encoder_param = selected_encoder_param;
		hdr.encoder_outlier = enc.outlier;
	}
	ret = cmp_hdr_serialize(&bs, &hdr);
	if (cmp_is_error_int(ret))
		return ret;

	compress_bound = cmp_compress_bound(src_size);
	if (cmp_is_error_int(compress_bound))
		compress_bound = ~0U;

	preprocess = preprocessing_get_method(selected_preprocessing);
	if (preprocess == NULL)
		return CMP_ERROR(PARAMS_INVALID);

	n_values = preprocess->init(src, src_size, ctx->work_buf, ctx->work_buf_size);
	if (cmp_is_error_int(n_values))
		return n_values;

	for (i = 0; i < n_values; i++) {
		int16_t const value = preprocess->process(i, src, ctx->work_buf);

		cmp_encoder_encode_s16(&enc, value, &bs);
		if (dst_capacity < compress_bound)
			if (cmp_is_error_int(bitstream_error(&bs)))
				break;

		if (model_is_needed) {
			uint16_t *model = ctx->work_buf;

			if (ctx->sequence_number == 0)
				model[i] = src[i];
			else
				model[i] = update_model(src[i], model[i], ctx->params.model_rate);
		}
	}

	if (ctx->params.checksum_enabled) {
		uint32_t const checksum = cmp_checksum(src, src_size);

		bitstream_pad_last_byte(&bs);
		bitstream_add_bits32(&bs, checksum, bitsizeof(checksum));
	}

	hdr.compressed_size = bitstream_flush(&bs);
	if (cmp_is_error_int(hdr.compressed_size))
		return hdr.compressed_size;

	/*
	 * Now that we have the final compressed size, rewind the bitstream and
	 * re-serialize the header with the correct cmp_size.
	 */
	ret = bitstream_rewind(&bs);
	if (cmp_is_error_int(ret))
		return ret;
	ret = cmp_hdr_serialize(&bs, &hdr);
	if (cmp_is_error_int(ret))
		return ret;

	ctx->sequence_number++;
	return hdr.compressed_size;
}


/* implements uncompressed fallback */
uint32_t cmp_compress_u16(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
			  const uint16_t *src, uint32_t src_size)
{
	uint32_t uncompressed_size = CMP_HDR_SIZE + src_size;
	enum cmp_preprocessing saved_preprocessing;
	enum cmp_encoder_type saved_encoder_type;
	uint32_t ret;

	if (ctx == NULL)
		return CMP_ERROR(GENERIC);

	if (ctx->magic != CMP_MAGIC)
		return CMP_ERROR(CONTEXT_INVALID);

	if (cmp_is_error_int(dst_capacity))
		return CMP_ERROR(GENERIC);

	if (ctx->params.checksum_enabled)
		uncompressed_size += CMP_CHECKSUM_SIZE;

	/* Skip fallback if disabled or output buffer too small for uncompressed */
	if (!ctx->params.uncompressed_fallback_enabled || dst_capacity < uncompressed_size)
		return compress_u16_engine(ctx, dst, dst_capacity, src, src_size);

	/*
	 * Try compression with restricted buffer size. If data doesn't compress
	 * well enough to fit in uncompressed_size bytes, we'll get a buffer
	 * overflow error and fall back to uncompressed storage.
	 */
	ret = compress_u16_engine(ctx, dst, uncompressed_size, src, src_size);
	if (cmp_get_error_code(ret) != CMP_ERR_DST_TOO_SMALL)
		return ret;

	/*
	 * Compression failed - fall back to uncompressed storage.
	 * Reset context to avoid corrupted model state, then temporarily
	 * switch to uncompressed mode.
	 */
	ret = cmp_reset(ctx);
	if (cmp_is_error_int(ret))
		return ret;
	saved_preprocessing = ctx->params.primary_preprocessing;
	saved_encoder_type = ctx->params.primary_encoder_type;
	ctx->params.primary_preprocessing = CMP_PREPROCESS_NONE;
	ctx->params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;

	ret = compress_u16_engine(ctx, dst, uncompressed_size, src, src_size);

	ctx->params.primary_preprocessing = saved_preprocessing;
	ctx->params.primary_encoder_type = saved_encoder_type;
	return ret;
}


static uint64_t cmp_get_new_identifier(void)
{
	uint32_t coarse = 0;
	uint16_t fine = 0;

	compile_time_assert(bitsizeof(coarse) + bitsizeof(fine) == CMP_HDR_BITS_IDENTIFIER,
			    cmp_hdr_identifier_timestamp_mismatch);

	g_get_timestamp(&coarse, &fine);

	return ((uint64_t)coarse << 16) | (uint64_t)fine;
}


uint32_t cmp_reset(struct cmp_context *ctx)
{
	if (ctx == NULL)
		return CMP_ERROR(GENERIC);

	if (ctx->magic != CMP_MAGIC)
		return CMP_ERROR(CONTEXT_INVALID);

	ctx->sequence_number = 0;
	ctx->identifier = cmp_get_new_identifier();
	ctx->model_size = 0;

	return CMP_ERROR(NO_ERROR);
}


void cmp_deinitialise(struct cmp_context *ctx)
{
	if (ctx)
		memset(ctx, 0, sizeof(*ctx));
}
