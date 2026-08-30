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
#include "../cmp_header.h"
#include "../cmp_errors.h"
#include "../common/sample_reader.h"
#include "../common/err_private.h"
#include "../common/bitstream_writer.h"
#include "../common/header_private.h"
#include "../common/bithacks.h"
#include "../common/compiler.h"
#include "../common/model.h"

enum { CMP_MAGIC = 34021395 }; /* arbitrary magic number I like */


/* Global identifier counter state */
static uint32_t g_identifier;


void cmp_hdr_set_identifier(uint32_t identifier)
{
	/* TODO: make this atomic */
	g_identifier = identifier;
}


unsigned int cmp_is_error(uint32_t code)
{
	return cmp_is_error_int(code);
}


uint32_t cmp_compress_bound(uint32_t src_size, enum cmp_type src_type)
{
	compile_time_assert(CMP_HDR_MAX_COMPRESSED_SIZE <= UINT32_MAX,
			    max_compressed_bound_exceeds_uint32_max);
	uint32_t num_samples;
	uint64_t bound;

	num_samples = cal_num_samples_round_up(src_size, src_type);
	if (cmp_is_error_int(num_samples))
		return num_samples;
	if (cal_packed_size(num_samples, src_type) > CMP_HDR_MAX_ORIGINAL_SIZE)
		return CMP_ERROR(HDR_ORIGINAL_TOO_LARGE);
	bound = CMP_HDR_SIZE + cmp_encoder_max_compressed_size(num_samples);

	if (bound > CMP_HDR_MAX_COMPRESSED_SIZE)
		return (CMP_ERROR(HDR_CMP_SIZE_TOO_LARGE));

	return (uint32_t)bound;
}


uint32_t cmp_cal_work_buf_size(const struct cmp_params *params, uint32_t src_size,
			       enum cmp_type src_type)
{
	const struct preprocessing_method *preprocess;
	uint32_t primary_work_buf_size, secondary_work_buf_size;
	uint32_t num_samples;

	if (params == NULL)
		return CMP_ERROR(GENERIC);

	if (params->primary_preprocessing == CMP_PREPROCESS_MODEL)
		return CMP_ERROR(PARAMS_INVALID);

	preprocess = preprocessing_get_method(params->primary_preprocessing);
	if (preprocess == NULL)
		return CMP_ERROR(PARAMS_INVALID);

	num_samples = cal_num_samples_round_up(src_size, src_type);
	if (cmp_is_error_int(num_samples))
		return num_samples;
	if (cal_packed_size(num_samples, src_type) > CMP_HDR_MAX_ORIGINAL_SIZE)
		return CMP_ERROR(HDR_ORIGINAL_TOO_LARGE);
	primary_work_buf_size = preprocess->get_work_buf_size(num_samples);

	if (params->secondary_iterations) {
		preprocess = preprocessing_get_method(params->secondary_preprocessing);
		if (preprocess == NULL)
			return CMP_ERROR(PARAMS_INVALID);
		secondary_work_buf_size = preprocess->get_work_buf_size(num_samples);
	} else {
		secondary_work_buf_size = 0;
	}

	return max_u32(primary_work_buf_size, secondary_work_buf_size);
}


uint32_t cmp_initialise(struct cmp_context *ctx, const struct cmp_params *params, void *work_buf,
			uint32_t work_buf_size)
{
	uint32_t const min_src_size = 2;
	uint32_t work_buf_size_needed;
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
	}

	if (params->secondary_iterations &&
	    params->secondary_preprocessing == CMP_PREPROCESS_MODEL &&
	    params->model_rate > CMP_MAX_MODEL_RATE)
		return CMP_ERROR(PARAMS_INVALID);

	work_buf_size_needed = cmp_cal_work_buf_size(params, min_src_size, CMP_I16);
	if (cmp_is_error_int(work_buf_size_needed))
		return work_buf_size_needed;

	if (work_buf_size_needed > 0) {
		if (!work_buf)
			return CMP_ERROR(WORK_BUF_NULL);
		if (work_buf_size == 0)
			return CMP_ERROR(WORK_BUF_TOO_SMALL);

		if (!is_aligned(work_buf, sizeof(uint16_t)))
			return CMP_ERROR(WORK_BUF_UNALIGNED);
	}

	ctx->params = *params;
	ctx->work_buf = work_buf;
	ctx->work_buf_size = work_buf_size;
	ctx->magic = CMP_MAGIC; /* add some magic */

	return cmp_reset(ctx);
}


/* fast shortcut for uncompressed data; assume model has sufficient size */
static void write_uncompressed(struct bitstream_writer *bs, const struct sample_desc *src_desc,
			       int16_t *model)
{
	uint32_t i;

	switch (src_desc->dtype) {
	case CMP_I16:
	case CMP_U16:
		bitstream_add_be16_array(bs, src_desc->data, src_desc->num_samples);
		break;
	case CMP_I16_IN_I32:
		bitstream_add_be16_in_32_array(bs, src_desc->data, src_desc->num_samples);
		break;
	case CMP_RAW12:
	default:
		/*
		 * For the CMP_RAW12 byte format, the packed size is the actual size
		 * and does not depend on the platform. Copy it as a byte array.
		 */
		bitstream_add_bytes(bs, src_desc->data, get_packed_size(src_desc));
		break;
	}

	if (model) {
		switch (src_desc->dtype) {
		case CMP_I16:
		case CMP_U16:
			memcpy(model, src_desc->data, get_packed_size(src_desc));
			break;
		case CMP_I16_IN_I32:
		case CMP_RAW12:
		default:
			for (i = 0; i < src_desc->num_samples; i++)
				model[i] = sample_read_i16(src_desc, i);
			break;
		}
	}
}


static int state_is_needed(const struct cmp_params *params)
{
	return params->secondary_preprocessing == CMP_PREPROCESS_MODEL &&
	       params->secondary_iterations != 0;
}


static enum cmp_type get_state_sample_type(const struct sample_desc *src_desc)
{
	switch (src_desc->dtype) {
	case CMP_U16:
	case CMP_RAW12:
		return CMP_U16;
	case CMP_I16:
	case CMP_I16_IN_I32:
	default:
		return CMP_I16;
	}
}


/* Main compression loop */
static uint32_t compress_engine(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
				const struct sample_desc *src_desc)
{
	uint32_t i, ret, n_values;
	enum cmp_preprocessing selected_preprocessing;
	enum cmp_encoder_type selected_encoder_type;
	uint32_t selected_encoder_param;
	uint32_t selected_outlier;
	uint32_t n_bits;
	struct bitstream_writer bs;
	struct cmp_encoder enc;
	const struct preprocessing_method *preprocess;
	int16_t *model = NULL;
	struct cmp_hdr hdr = { 0 };

	if (ctx->sequence_number > ctx->params.secondary_iterations) {
		/* Note: cmp_reset() resets ctx->sequence_number to 0. */
		ret = cmp_reset(ctx);
		if (cmp_is_error_int(ret))
			return ret;
	}

	if (ctx->sequence_number == 0) {
		selected_preprocessing = ctx->params.primary_preprocessing;
		selected_encoder_type = ctx->params.primary_encoder_type;
		selected_encoder_param = ctx->params.primary_encoder_param;
		selected_outlier = ctx->params.primary_encoder_outlier;
		ctx->state_num_samples = src_desc->num_samples;
		ctx->state_dtype = get_state_sample_type(src_desc);
	} else {
		selected_preprocessing = ctx->params.secondary_preprocessing;
		selected_encoder_type = ctx->params.secondary_encoder_type;
		selected_encoder_param = ctx->params.secondary_encoder_param;
		selected_outlier = ctx->params.secondary_encoder_outlier;
		/*
		 * Preprocessing state requires the original input layout to
		 * remain unchanged until the context is reset.
		 */
		if (state_is_needed(&ctx->params) &&
		    (src_desc->num_samples != ctx->state_num_samples ||
		     get_state_sample_type(src_desc) != ctx->state_dtype))
			return CMP_ERROR(SRC_MISMATCH);
	}

	if (state_is_needed(&ctx->params)) {
		if (ctx->work_buf_size < src_desc->num_samples * sizeof(uint16_t))
			return CMP_ERROR(WORK_BUF_TOO_SMALL);
		model = ctx->work_buf;
	}

	ret = bitstream_writer_init(&bs, dst, dst_capacity);
	if (cmp_is_error_int(ret))
		return ret;

	/*
	 * Using UNCOMPRESSED with preprocessing is mainly useful for testing
	 * the preprocessing. Write the int16_t preprocessing output as 16-bit
	 * big endian values to keep the output format consistent (also for
	 * RAW12 data).
	 */
	if (selected_preprocessing != CMP_PREPROCESS_NONE &&
	    selected_encoder_type == CMP_ENCODER_UNCOMPRESSED)
		n_bits = 16;
	else
		n_bits = preprocessing_get_output_bits(selected_preprocessing, src_desc);
	ret = cmp_encoder_init(&enc, selected_encoder_type, selected_encoder_param,
			       selected_outlier, n_bits);
	if (cmp_is_error_int(ret))
		return ret;

	hdr.version = CMP_VERSION_NUMBER;
	hdr.original_size = get_packed_size(src_desc);
	hdr.compressed_size = 0; /* place holder, not know right now */
	if (ctx->params.checksum_enabled)
		hdr.checksum = cmp_hdr_checksum_int(src_desc);
	else
		hdr.checksum = 0;
	hdr.identifier = ctx->identifier;
	hdr.sequence_number = ctx->sequence_number;
	hdr.preprocessing = selected_preprocessing;
	hdr.encoder_type = selected_encoder_type;
	hdr.original_dtype = src_desc->dtype;
	if (selected_preprocessing == CMP_PREPROCESS_MODEL)
		hdr.preprocess_param = ctx->params.model_rate;
	else
		hdr.preprocess_param = ctx->params.secondary_iterations;
	if (selected_encoder_type != CMP_ENCODER_UNCOMPRESSED) {
		hdr.encoder_param = selected_encoder_param;
		hdr.encoder_outlier = enc.outlier;
	}
	ret = cmp_hdr_serialize(&bs, &hdr);
	if (cmp_is_error_int(ret))
		return ret;

	if (selected_preprocessing == CMP_PREPROCESS_NONE &&
	    selected_encoder_type == CMP_ENCODER_UNCOMPRESSED) {
		write_uncompressed(&bs, src_desc, model);
	} else {
		uint32_t compress_bound = (uint32_t)min_u64(
			UINT32_MAX, cmp_encoder_max_compressed_size(src_desc->num_samples));

		preprocess = preprocessing_get_method(selected_preprocessing);
		if (preprocess == NULL)
			return CMP_ERROR(PARAMS_INVALID);

		n_values = preprocess->init(src_desc, ctx->work_buf, ctx->work_buf_size);
		if (cmp_is_error_int(n_values))
			return n_values;

		for (i = 0; i < n_values; i++) {
			int16_t const value = preprocess->process(i, src_desc, ctx->work_buf);

			cmp_encoder_encode(&enc, value, &bs);
			if (dst_capacity < compress_bound)
				if (cmp_is_error_int(bitstream_error(&bs)))
					break;

			if (model) {
				if (ctx->sequence_number == 0)
					model[i] = sample_read_i16(src_desc, i);
				else
					model[i] = update_model(sample_read_i16(src_desc, i),
								model[i],
								(int)ctx->params.model_rate,
								src_desc->dtype);
			}
		}
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


/* Implements uncompressed fallback. */
static uint32_t cmp_compress_generic(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
				     const struct sample_desc *src_desc)
{
	uint32_t uncompressed_size = CMP_HDR_SIZE + get_packed_size(src_desc);
	enum cmp_preprocessing saved_preprocessing;
	enum cmp_encoder_type saved_encoder_type;
	uint32_t ret;

	if (ctx == NULL)
		return CMP_ERROR(GENERIC);

	if (ctx->magic != CMP_MAGIC)
		return CMP_ERROR(CONTEXT_INVALID);

	if (cmp_is_error_int(dst_capacity))
		return CMP_ERROR(GENERIC);

	/* Skip fallback if disabled or output buffer too small for uncompressed */
	if (!ctx->params.uncompressed_fallback_enabled || dst_capacity < uncompressed_size)
		return compress_engine(ctx, dst, dst_capacity, src_desc);

	/*
	 * Try compression with restricted buffer size. If data doesn't compress
	 * well enough to fit in uncompressed_size bytes, we'll get a buffer
	 * overflow error and fall back to uncompressed storage.
	 */
	ret = compress_engine(ctx, dst, uncompressed_size, src_desc);
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

	ret = compress_engine(ctx, dst, uncompressed_size, src_desc);

	ctx->params.primary_preprocessing = saved_preprocessing;
	ctx->params.primary_encoder_type = saved_encoder_type;
	return ret;
}


uint32_t cmp_compress_u16(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
			  const uint16_t *src, uint32_t src_size)
{
	uint32_t error;
	struct sample_desc src_desc;

	error = sample_read_src_init(&src_desc, src, src_size, CMP_U16);
	if (cmp_is_error(error))
		return error;

	return cmp_compress_generic(ctx, dst, dst_capacity, &src_desc);
}


uint32_t cmp_compress_i16(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
			  const int16_t *src, uint32_t src_size)
{
	uint32_t error;
	struct sample_desc src_desc;

	error = sample_read_src_init(&src_desc, src, src_size, CMP_I16);
	if (cmp_is_error(error))
		return error;

	return cmp_compress_generic(ctx, dst, dst_capacity, &src_desc);
}


uint32_t cmp_compress_i16_in_i32(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
				 const int32_t *src, uint32_t src_size)
{
	uint32_t error;
	struct sample_desc src_desc;

	error = sample_read_src_init(&src_desc, src, src_size, CMP_I16_IN_I32);
	if (cmp_is_error(error))
		return error;

	return cmp_compress_generic(ctx, dst, dst_capacity, &src_desc);
}


uint32_t cmp_compress_raw12(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
			    const uint8_t *src, uint32_t src_size)
{
	uint32_t error;
	struct sample_desc src_desc;

	error = sample_read_src_init(&src_desc, src, src_size, CMP_RAW12);
	if (cmp_is_error(error))
		return error;

	return cmp_compress_generic(ctx, dst, dst_capacity, &src_desc);
}


static uint32_t cmp_get_new_identifier(void)
{
	/* TODO: make this atomic */
	return g_identifier++;
}


uint32_t cmp_reset(struct cmp_context *ctx)
{
	if (ctx == NULL)
		return CMP_ERROR(GENERIC);

	if (ctx->magic != CMP_MAGIC)
		return CMP_ERROR(CONTEXT_INVALID);

	ctx->sequence_number = 0;
	ctx->identifier = cmp_get_new_identifier();
	ctx->state_num_samples = 0;
	ctx->state_dtype = CMP_I16;

	return CMP_ERROR(NO_ERROR);
}


void cmp_deinitialise(struct cmp_context *ctx)
{
	if (ctx)
		memset(ctx, 0, sizeof(*ctx));
}
