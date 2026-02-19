/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Compression Context Initialisation Tests
 */

#include <string.h>

#include <unity.h>
#include "test_common.h"

#include "../lib/cmp.h"
#include "../lib/cmp_header.h"
#include "../lib/cmp_errors.h"


void test_successful_compression_initialisation_without_work_buf(void)
{
	struct cmp_context ctx;
	struct cmp_params par = { 0 };

	uint32_t return_val = cmp_initialise(&ctx, &par, NULL, 3);

	TEST_ASSERT_CMP_SUCCESS(return_val);
}


void test_successful_compression_initialisation_with_work_buf(void)
{
	struct cmp_context ctx;
	struct cmp_params par = { 0 };
	uint16_t work_buf[2];

	uint32_t return_val = cmp_initialise(&ctx, &par, work_buf, sizeof(work_buf));

	TEST_ASSERT_CMP_SUCCESS(return_val);
}


void test_detect_null_context_initialisation(void)
{
	struct cmp_params const par = { 0 };
	uint32_t return_val;

	return_val = cmp_initialise(NULL, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_GENERIC, return_val);
}


/*
 * Compression Parameters Tests
 */

void test_detect_null_parameters_initialisation(void)
{
	struct cmp_context const ctx_all_zero = { 0 };
	uint16_t work_buf[2];
	struct cmp_context ctx;
	uint32_t return_val;

	memset(&ctx, 0xFF, sizeof(ctx));

	return_val = cmp_initialise(&ctx, NULL, work_buf, sizeof(work_buf));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_GENERIC, return_val);
	TEST_ASSERT_EQUAL_MEMORY(&ctx_all_zero, &ctx, sizeof(ctx));
}


#define INVALID_PREPROCESSING ((enum cmp_preprocessing)0xFFFF)
void test_detect_invalid_primary_preprocessing_initialization(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.primary_preprocessing = INVALID_PREPROCESSING;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_detect_invalid_primary_model_preprocessing_initialization(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.primary_preprocessing = CMP_PREPROCESS_MODEL;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_detect_invalid_secondary_preprocessing_initialization(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.secondary_iterations = 1;
	par.secondary_preprocessing = INVALID_PREPROCESSING;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_ignore_invalid_secondary_preprocessing_when_not_used(void)
{
	const uint16_t src[2] = { 0x0001, 0x0203 };
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src))];
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val, cmp_size;

	par.secondary_iterations = 0;
	par.secondary_preprocessing = INVALID_PREPROCESSING;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);
	cmp_size = cmp_compress_u16(&ctx, dst, sizeof(dst), src, sizeof(src));

	TEST_ASSERT_CMP_SUCCESS(return_val);
	TEST_ASSERT_CMP_SUCCESS(cmp_size);
	{
		struct cmp_hdr expected_hdr = { 0 };

		expected_hdr.compressed_size = cmp_size;
		expected_hdr.original_size = sizeof(src);
		expected_hdr.original_dtype = CMP_U16;
		TEST_ASSERT_CMP_HDR(dst, cmp_size, expected_hdr);
	}
}


#define INVALID_ENCODER ((enum cmp_encoder_type)0xFFFF)
void test_detect_invalid_primary_encoder_initialisation(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.primary_encoder_type = INVALID_ENCODER;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_detect_invalid_secondary_endoder_initialisation(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.secondary_iterations = 1;
	par.secondary_encoder_type = INVALID_ENCODER;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_ignore_invalid_secondary_encodder_when_not_used(void)
{
	const uint16_t src[2] = { 0x0001, 0x0203 };
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src))];
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val, cmp_size;

	par.secondary_iterations = 0;
	par.secondary_encoder_type = INVALID_ENCODER;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);
	cmp_size = cmp_compress_u16(&ctx, dst, sizeof(dst), src, sizeof(src));

	TEST_ASSERT_CMP_SUCCESS(return_val);
	TEST_ASSERT_CMP_SUCCESS(cmp_size);
	{
		struct cmp_hdr expected_hdr = { 0 };

		expected_hdr.compressed_size = cmp_size;
		expected_hdr.original_size = sizeof(src);
		expected_hdr.original_dtype = CMP_U16;
		TEST_ASSERT_CMP_HDR(dst, cmp_size, expected_hdr);
	}
}


void test_detects_invalid_secondary_iterations_value(void)
{
	uint32_t return_value;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.secondary_iterations = 256;

	return_value = cmp_initialise(&ctx, &params, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_value);
}


void test_detect_invalid_primary_golomb_encoder_parameter(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	par.primary_encoder_param = UINT16_MAX + 1;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);

	/* zero is also invalid */
	par.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	par.primary_encoder_param = 0;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_ignore_invalid_primary_golomb_encoder_parameter_when_not_used(void)
{
	const uint16_t src[2] = { 0x0001, 0x0203 };
	DST_ALIGNED_U8 dst[CMP_HDR_SIZE + sizeof(src)];
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val, cmp_size;

	par.primary_encoder_param = UINT16_MAX + 1;
	par.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	par.primary_preprocessing = CMP_PREPROCESS_DIFF;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);
	cmp_size = cmp_compress_u16(&ctx, dst, sizeof(dst), src, sizeof(src));

	TEST_ASSERT_CMP_SUCCESS(return_val);
	TEST_ASSERT_CMP_SUCCESS(cmp_size);
	{
		struct cmp_hdr expected_hdr = { 0 };

		expected_hdr.compressed_size = cmp_size;
		expected_hdr.original_size = sizeof(src);
		expected_hdr.original_dtype = CMP_U16;
		expected_hdr.preprocessing = CMP_PREPROCESS_DIFF;
		TEST_ASSERT_CMP_HDR(dst, cmp_size, expected_hdr);
	}
}


void test_detect_invalid_secondary_golomb_encoder_parameter(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.secondary_iterations = 1;
	par.secondary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	par.secondary_encoder_param = UINT16_MAX + 1;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);

	/* zero is also invalid */
	par.secondary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	par.secondary_encoder_param = 0;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_ignore_invalid_secondary_golomb_encoder_parameter_when_not_used(void)
{
	const uint16_t src[2] = { 0x0001, 0x0203 };
	DST_ALIGNED_U8 dst[CMP_HDR_SIZE + sizeof(src)];
	DST_ALIGNED_U8 dst2[CMP_HDR_SIZE + sizeof(src)];
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val, cmp_size, cmp_size2;

	par.secondary_encoder_param = UINT16_MAX + 1;
	par.secondary_iterations = 1;
	par.secondary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	par.secondary_preprocessing = CMP_PREPROCESS_DIFF;
	par.primary_preprocessing = CMP_PREPROCESS_DIFF;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);
	cmp_size = cmp_compress_u16(&ctx, dst, sizeof(dst), src, sizeof(src));
	cmp_size2 = cmp_compress_u16(&ctx, dst2, sizeof(dst2), src, sizeof(src));

	TEST_ASSERT_CMP_SUCCESS(return_val);
	TEST_ASSERT_CMP_SUCCESS(cmp_size);
	TEST_ASSERT_CMP_SUCCESS(cmp_size2);
	{
		struct cmp_hdr expected_hdr = { 0 };

		expected_hdr.compressed_size = cmp_size;
		expected_hdr.original_size = sizeof(src);
		expected_hdr.original_dtype = CMP_U16;
		expected_hdr.preprocessing = CMP_PREPROCESS_DIFF;
		expected_hdr.preprocess_param = 1;
		TEST_ASSERT_CMP_HDR(dst, cmp_size, expected_hdr);
		expected_hdr.sequence_number = 1;
		TEST_ASSERT_CMP_HDR(dst2, cmp_size2, expected_hdr);
	}
}


void test_detects_invalid_model_rate(void)
{
	uint32_t return_value;
	struct cmp_context ctx;
	uint16_t work_buf[4];
	struct cmp_params params = { 0 };

	params.model_rate = 17;
	params.secondary_iterations = 1;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;

	return_value = cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_value);
}


void test_ignore_invalid_model_rate_when_not_used(void)
{
	const uint16_t src[2] = { 0x0001, 0x0203 };
	DST_ALIGNED_U8 dst[CMP_HDR_SIZE + sizeof(src)];
	uint16_t work_buf[4];
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	uint32_t return_value, cmp_size;

	params.model_rate = UINT32_MAX;
	params.secondary_iterations = 1;
	params.secondary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;

	return_value = cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf));
	cmp_size = cmp_compress_u16(&ctx, dst, sizeof(dst), src, sizeof(src));

	TEST_ASSERT_CMP_SUCCESS(return_value);
	TEST_ASSERT_CMP_SUCCESS(cmp_size);
	{
		struct cmp_hdr expected_hdr = { 0 };

		expected_hdr.compressed_size = cmp_size;
		expected_hdr.original_size = sizeof(src);
		expected_hdr.original_dtype = CMP_U16;
		expected_hdr.preprocessing = CMP_PREPROCESS_DIFF;
		expected_hdr.preprocess_param = 1;
		TEST_ASSERT_CMP_HDR(dst, cmp_size, expected_hdr);
	}
}


/*
 * Work Buffer Initialisation Tests
 */

void test_detect_missing_iwt_work_buffer(void)
{
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	uint32_t return_value;

	params.primary_preprocessing = CMP_PREPROCESS_IWT;

	return_value = cmp_initialise(&ctx, &params, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_NULL, return_value);
}


void test_detect_unaligned_iwt_work_buffer(void)
{
	uint8_t work_buf[4];
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	uint32_t return_value;

	params.primary_preprocessing = CMP_PREPROCESS_IWT;

	return_value = cmp_initialise(&ctx, &params, work_buf + 1, 3);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_UNALIGNED, return_value);
}


void test_detect_missing_model_work_buffer(void)
{
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	uint32_t return_value;

	params.secondary_iterations = 1;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;

	return_value = cmp_initialise(&ctx, &params, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_NULL, return_value);
}


void test_detect_unaligned_model_work_buffer(void)
{
	uint8_t work_buf[4];
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	uint32_t return_value;

	params.secondary_iterations = 1;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;

	return_value = cmp_initialise(&ctx, &params, work_buf + 1, 3);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_UNALIGNED, return_value);
}


void test_detect_0_size_work_buffer(void)
{
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	uint32_t work_buf[1];
	uint32_t return_value;

	params.secondary_iterations = 1;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;

	return_value = cmp_initialise(&ctx, &params, work_buf, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_TOO_SMALL, return_value);
}


void test_params_invalid_error_has_priority_over_work_buffer_error(void)
{
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	uint32_t return_value;

	params.model_rate = 1000;
	params.secondary_iterations = 1;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;

	return_value = cmp_initialise(&ctx, &params, NULL, 10);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_value);
}


void test_init_fails_if_workbufer_size_is_a_propagated_error(void)
{
	struct cmp_params params_invalid = { 0 };
	struct cmp_params params_valid = { 0 };
	struct cmp_context ctx;
	uint32_t buf_size_error_code, return_value;
	uint16_t work_buf_dummy[1];

	params_invalid.primary_preprocessing = INVALID_PREPROCESSING;
	params_valid.primary_preprocessing = CMP_PREPROCESS_IWT;

	buf_size_error_code = cmp_cal_work_buf_size(&params_invalid, 41);
	return_value = cmp_initialise(&ctx, &params_valid, work_buf_dummy, buf_size_error_code);

	TEST_ASSERT_CMP_FAILURE(buf_size_error_code);
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_GENERIC, return_value);
}
