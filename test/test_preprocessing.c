/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Pre-Processing Tests
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <unity.h>
#include "test_common.h"

#include "../lib/cmp.h"
#include "../lib/cmp_header.h"
#include "../lib/common/compiler.h"


static void assert_preprocessing_data(const int16_t *expected_output, uint32_t num_elements,
				      const uint8_t *compressed_data)
{
	int16_t output[8];
	const uint8_t *p = cmp_hdr_get_cmp_data(compressed_data);
	uint32_t i;

	TEST_ASSERT_LESS_OR_EQUAL(ARRAY_SIZE(output), num_elements);
	for (i = 0; i < num_elements; i++) /* convert to system endianness */
		output[i] = (int16_t)(p[i * 2] << 8) | (int16_t)(p[i * 2 + 1]);
	TEST_ASSERT_EQUAL_INT16_ARRAY(expected_output, output, num_elements);
}


#define DIFF_PREPROC_SRC_VALUES 0x0001, 0x0003, 0x0000, 0xffff, 0x0000, 0x7fff, 0x8000, 0xfffb
const uint16_t test_diff_u16[8] = { DIFF_PREPROC_SRC_VALUES };
const int16_t test_diff_i16[8] = { DIFF_PREPROC_SRC_VALUES };
const int32_t test_diff_i16_in_i32[8] = { DIFF_PREPROC_SRC_VALUES };

TEST_CASE(compress_u16_wrapper, ARRAY_AND_SIZE(test_diff_u16))
TEST_CASE(compress_i16_wrapper, ARRAY_AND_SIZE(test_diff_i16))
TEST_CASE(compress_i16_in_i32_wrapper, ARRAY_AND_SIZE(test_diff_i16_in_i32))

void test_diff_preprocessing_for_multiple_values(compress_func_t compress_func, const void *src,
						 uint32_t src_size)
{
	const int16_t expected_diff[ARRAY_SIZE(test_diff_u16)] = { 1, 2,         -3, -1,
								   1, INT16_MAX, 1,  0x7FFB };
	DST_ALIGNED_U8 output_buf[CMP_UNCOMPRESSED_BOUND(sizeof(expected_diff))];
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_hdr expected_hdr = { 0 };
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	output_size = compress_func(&ctx, output_buf, sizeof(output_buf), src, src_size);

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(sizeof(expected_diff)), output_size);
	assert_preprocessing_data(expected_diff, ARRAY_SIZE(expected_diff), output_buf);
	expected_hdr.compressed_size = output_size;
	expected_hdr.original_size = sizeof(test_diff_u16);
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.primary_preprocessing;
	TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
}


const int16_t g_iwt_input_1[] = { 42 };
const int32_t g_iwt_input_1_i32[] = { 42 };
const int16_t g_iwt_exp_out_1[] = { 42 };

const int16_t g_iwt_input_2[2] = { -23809, 23901 };
const int32_t g_iwt_input_2_i32[2] = { -23809, 23901 };
const int16_t g_iwt_exp_out_2[2] = { -32722, -17826 };

const int16_t g_iwt_input_5[5] = { -1, 2, -3, 4, -5 };
const int32_t g_iwt_input_5_i32[5] = { -1, 2, -3, 4, -5 };
const int16_t g_iwt_exp_out_5[5] = { 0, 4, 0, 8, -2 };

const int16_t g_iwt_input_7[7] = { 0, 0, 2, 0, 0, 0, 0 };
const int32_t g_iwt_input_7_i32[7] = { 0, 0, 2, 0, 0, 0, 0 };
const int16_t g_iwt_exp_out_7[7] = { -1, -1, 2, -1, -1, 0, 1 };

const int16_t g_iwt_input_8[8] = { -3, 2, -1, 3, -2, 5, 0, 7 };
const int32_t g_iwt_input_8_i32[8] = { -3, 2, -1, 3, -2, 5, 0, 7 };
const int16_t g_iwt_exp_out_8[8] = { 0, 4, 2, 5, 1, 6, 3, 7 };

TEST_CASE(compress_u16_wrapper, ARRAY_AND_SIZE(g_iwt_input_1), ARRAY_AND_SIZE(g_iwt_exp_out_1))
TEST_CASE(compress_i16_wrapper, ARRAY_AND_SIZE(g_iwt_input_1), ARRAY_AND_SIZE(g_iwt_exp_out_1))
TEST_CASE(compress_i16_in_i32_wrapper, ARRAY_AND_SIZE(g_iwt_input_1_i32),
	  ARRAY_AND_SIZE(g_iwt_exp_out_1))

TEST_CASE(compress_u16_wrapper, ARRAY_AND_SIZE(g_iwt_input_2), ARRAY_AND_SIZE(g_iwt_exp_out_2))
TEST_CASE(compress_i16_wrapper, ARRAY_AND_SIZE(g_iwt_input_2), ARRAY_AND_SIZE(g_iwt_exp_out_2))
TEST_CASE(compress_i16_in_i32_wrapper, ARRAY_AND_SIZE(g_iwt_input_2_i32),
	  ARRAY_AND_SIZE(g_iwt_exp_out_2))

TEST_CASE(compress_u16_wrapper, ARRAY_AND_SIZE(g_iwt_input_5), ARRAY_AND_SIZE(g_iwt_exp_out_5))
TEST_CASE(compress_i16_wrapper, ARRAY_AND_SIZE(g_iwt_input_5), ARRAY_AND_SIZE(g_iwt_exp_out_5))
TEST_CASE(compress_i16_in_i32_wrapper, ARRAY_AND_SIZE(g_iwt_input_5_i32),
	  ARRAY_AND_SIZE(g_iwt_exp_out_5))

TEST_CASE(compress_u16_wrapper, ARRAY_AND_SIZE(g_iwt_input_7), ARRAY_AND_SIZE(g_iwt_exp_out_7))
TEST_CASE(compress_i16_wrapper, ARRAY_AND_SIZE(g_iwt_input_7), ARRAY_AND_SIZE(g_iwt_exp_out_7))
TEST_CASE(compress_i16_in_i32_wrapper, ARRAY_AND_SIZE(g_iwt_input_7_i32),
	  ARRAY_AND_SIZE(g_iwt_exp_out_7))

TEST_CASE(compress_u16_wrapper, ARRAY_AND_SIZE(g_iwt_input_8), ARRAY_AND_SIZE(g_iwt_exp_out_8))
TEST_CASE(compress_i16_wrapper, ARRAY_AND_SIZE(g_iwt_input_8), ARRAY_AND_SIZE(g_iwt_exp_out_8))
TEST_CASE(compress_i16_in_i32_wrapper, ARRAY_AND_SIZE(g_iwt_input_8_i32),
	  ARRAY_AND_SIZE(g_iwt_exp_out_8))

void test_iwt_transform(compress_func_t compress_func, const void *input, uint32_t input_size,
			const int16_t *exp_output, uint32_t exp_size)
{
	uint32_t dst_size;
	struct test_env *e;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_IWT;
	e = make_env(&params, exp_size);

	dst_size = compress_func(&e->ctx, e->dst, e->dst_cap, input, input_size);

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(exp_size), dst_size);
	assert_preprocessing_data(exp_output, exp_size / sizeof(int16_t), e->dst);
	expected_hdr.compressed_size = CMP_HDR_SIZE + exp_size;
	expected_hdr.original_size = exp_size;
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.primary_preprocessing;
	TEST_ASSERT_CMP_HDR(e->dst, dst_size, expected_hdr);

	free_env(e);
}


TEST_CASE(compress_u16_wrapper)
TEST_CASE(compress_i16_wrapper)
void test_model_preprocessing_for_multiple_values(compress_func_t compress_func)
{
	const uint16_t start_model[] = { 0, 1, 10 };
	const uint16_t data[ARRAY_SIZE(start_model)] = { 1, 3, 5 };
	const int16_t expected_output[ARRAY_SIZE(start_model)] = { 1, 2, -5 };
	uint32_t dst_size;
	struct test_env *e;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 1;
	e = make_env(&params, sizeof(data));

	TEST_ASSERT_CMP_SUCCESS(
		compress_func(&e->ctx, e->dst, e->dst_cap, start_model, sizeof(start_model)));
	dst_size = compress_func(&e->ctx, e->dst, e->dst_cap, data, sizeof(data));

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(sizeof(expected_output)), dst_size);
	assert_preprocessing_data(expected_output, ARRAY_SIZE(expected_output), e->dst);
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = sizeof(data);
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.secondary_preprocessing;
	expected_hdr.sequence_number = 1;
	TEST_ASSERT_CMP_HDR(e->dst, dst_size, expected_hdr);

	free_env(e);
}


void test_model_preprocessing_for_multiple_i16_in_i32_values(void)
{
	const int32_t start_model[] = { 0, 1, 10, -4 };
	const int32_t data[ARRAY_SIZE(start_model)] = { 1, 3, 5, -1 };
	const int16_t expected_output[ARRAY_SIZE(start_model)] = { 1, 2, -5, 3 };

	uint16_t work_buf[ARRAY_SIZE(start_model)];
	DST_ALIGNED_U8 output_buf[CMP_UNCOMPRESSED_BOUND(sizeof(expected_output))];
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_hdr expected_hdr = { 0 };
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));
	TEST_ASSERT_CMP_SUCCESS(cmp_compress_i16_in_i32(&ctx, output_buf, sizeof(output_buf),
							start_model, sizeof(start_model)));

	output_size =
		cmp_compress_i16_in_i32(&ctx, output_buf, sizeof(output_buf), data, sizeof(data));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(sizeof(expected_output)), output_size);
	assert_preprocessing_data(expected_output, ARRAY_SIZE(expected_output), output_buf);

	expected_hdr.compressed_size = output_size;
	expected_hdr.original_size = sizeof(expected_output);
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.secondary_preprocessing;
	expected_hdr.sequence_number = 1;
	TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
}


const uint16_t model_input1_u16[5] = { 0, 2, 21, 1, UINT16_MAX };         /* this is the model */
const uint16_t model_input2_u16[5] = { 1, 3, 5, UINT16_MAX, UINT16_MAX }; /* this is the value */
const uint16_t model_input3_u16[5] = { 0 }; /* when m=0 -> o=-m (o=v-m)  */
const int16_t expec_output_u16[5] = { 0, -2, -6, (int16_t)-61439, (uint16_t)-UINT16_MAX };

const int16_t model_input1_i16[7] = { 15, 2, 21, 0, 0, INT16_MIN, INT16_MAX };
const int16_t model_input2_i16[7] = { -2, 3, 5, -1, 0, INT16_MIN, INT16_MAX };
const int16_t model_input3_i16[7] = { 0 };
const int16_t expected_out_i16[7] = { 1, -2, -6, 1, 0, (int16_t)-INT16_MIN, -INT16_MAX };

const int32_t model_input1_i16_in_i32[7] = { 15, 2, 21, 0, 0, INT16_MIN, INT16_MAX };
const int32_t model_input2_i16_in_i32[7] = { -2, 3, 5, -1, 0, INT16_MIN, INT16_MAX };
const int32_t model_input3_i16_in_i32[7] = { 0 };

TEST_CASE(compress_u16_wrapper, model_input1_u16, model_input2_u16,
	  ARRAY_AND_SIZE(model_input3_u16), ARRAY_AND_SIZE(expec_output_u16))
TEST_CASE(compress_i16_wrapper, model_input1_i16, model_input2_i16,
	  ARRAY_AND_SIZE(model_input3_i16), ARRAY_AND_SIZE(expected_out_i16))
TEST_CASE(compress_i16_in_i32_wrapper, model_input1_i16_in_i32, model_input2_i16_in_i32,
	  ARRAY_AND_SIZE(model_input3_i16_in_i32), ARRAY_AND_SIZE(expected_out_i16))

void test_model_updates_correctly(compress_func_t compress_func, const void *src1, const void *src2,
				  const void *src3, uint32_t src_size, const void *pre_data_exp,
				  uint32_t pre_data_exp_size)
{
	uint32_t output_size;
	struct test_env *e;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.model_rate = 1;
	params.secondary_iterations = 2;
	e = make_env(&params, pre_data_exp_size);

	TEST_ASSERT_CMP_SUCCESS(compress_func(&e->ctx, e->dst, e->dst_cap, src1, src_size));
	TEST_ASSERT_CMP_SUCCESS(compress_func(&e->ctx, e->dst, e->dst_cap, src2, src_size));
	output_size = compress_func(&e->ctx, e->dst, e->dst_cap, src3, src_size);

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(pre_data_exp_size), output_size);
	assert_preprocessing_data(pre_data_exp, pre_data_exp_size / 2, e->dst);
	expected_hdr.compressed_size = output_size;
	expected_hdr.original_size = pre_data_exp_size;
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.secondary_preprocessing;
	expected_hdr.preprocess_param = 1;
	expected_hdr.sequence_number = 2;
	TEST_ASSERT_CMP_HDR(e->dst, output_size, expected_hdr);

	free_env(e);
}


TEST_CASE(compress_u16_wrapper, ARRAY_AND_SIZE(test_dummy_u16))
TEST_CASE(compress_i16_wrapper, ARRAY_AND_SIZE(test_dummy_i16))
TEST_CASE(compress_i16_in_i32_wrapper, ARRAY_AND_SIZE(test_dummy_i16_in_i32))
void test_primary_preprocessing_after_max_secondary_iterations(compress_func_t compress_func,
							       const void *src, uint32_t src_size)
{
	uint32_t output_size;
	struct test_env *e;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 2;
	e = make_env(&params, sizeof(test_dummy_i16));

	TEST_ASSERT_CMP_SUCCESS(compress_func(&e->ctx, e->dst, e->dst_cap, src, src_size));
	TEST_ASSERT_CMP_SUCCESS(compress_func(&e->ctx, e->dst, e->dst_cap, src, src_size));
	TEST_ASSERT_CMP_SUCCESS(compress_func(&e->ctx, e->dst, e->dst_cap, src, src_size));
	output_size = compress_func(&e->ctx, e->dst, e->dst_cap, src, src_size);

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(test_dummy_i16), output_size);
	assert_preprocessing_data(test_dummy_i16, ARRAY_SIZE(test_dummy_i16), e->dst);
	expected_hdr.compressed_size = output_size;
	expected_hdr.original_size = sizeof(test_dummy_i16);
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = CMP_PREPROCESS_NONE;
	expected_hdr.preprocess_param = params.secondary_iterations;
	TEST_ASSERT_CMP_HDR(e->dst, output_size, expected_hdr);

	free_env(e);
}


void test_detect_invalid_primary_preprocessing_model_usage(void)
{
	struct cmp_context ctx;
	uint16_t work_buf[4];
	uint32_t return_val;
	struct cmp_params par = { 0 };

	par.primary_preprocessing = CMP_PREPROCESS_MODEL;

	return_val = cmp_initialise(&ctx, &par, work_buf, sizeof(work_buf));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


TEST_CASE(compress_u16_wrapper)
TEST_CASE(compress_i16_wrapper)
TEST_CASE(compress_i16_in_i32_wrapper)
void test_unrelated_compressions_get_unique_identifiers(compress_func_t compress_func)
{
	const int32_t src1[4] = { 0 };
	const int32_t src2[4] = { 0 };
	uint16_t work_buf1[ARRAY_SIZE(src1) * 2];
	uint16_t work_buf2[ARRAY_SIZE(src2) * 2];
	DST_ALIGNED_U8 dst_buf1[CMP_UNCOMPRESSED_BOUND(sizeof(src1))];
	DST_ALIGNED_U8 dst_buf2[CMP_UNCOMPRESSED_BOUND(sizeof(src2))];
	uint32_t dst_size1, dst_size2;
	struct cmp_hdr hdr1, hdr2;
	struct cmp_context ctx1, ctx2;
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 10;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx1, &params, work_buf1, sizeof(work_buf1)));
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx2, &params, work_buf2, sizeof(work_buf2)));

	dst_size1 = compress_func(&ctx1, dst_buf1, sizeof(dst_buf1), src1, sizeof(src1));
	dst_size2 = compress_func(&ctx2, dst_buf2, sizeof(dst_buf2), src2, sizeof(src2));

	TEST_ASSERT_CMP_SUCCESS(dst_size1);
	TEST_ASSERT_CMP_SUCCESS(dst_size2);
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(dst_buf1, dst_size1, &hdr1));
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(dst_buf2, dst_size2, &hdr2));
	TEST_ASSERT_NOT_EQUAL(hdr1.identifier, hdr2.identifier);
}


TEST_CASE(compress_u16_wrapper)
TEST_CASE(compress_i16_wrapper)
void test_detect_to_small_work_buffer_in_model_preprocessing(compress_func_t compress_func)
{
	const uint16_t src[4] = { 0 };
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src))];
	uint16_t work_buf[ARRAY_SIZE(src) - 1];
	uint32_t work_buf_size, return_code;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 1;
	work_buf_size = cmp_cal_work_buf_size(&params, sizeof(src));
	TEST_ASSERT_LESS_THAN(work_buf_size, sizeof(work_buf));
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));

	return_code = compress_func(&ctx, dst, sizeof(dst), src, sizeof(src));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_TOO_SMALL, return_code);
}


void test_detect_to_small_work_buffer_in_model_preprocessing_i16_in_i32(void)
{
	const int32_t src[4] = { 0 };
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(ARRAY_SIZE(src) * sizeof(int16_t))];
	uint16_t work_buf[ARRAY_SIZE(src) - 1];
	uint32_t work_buf_size, return_code;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 1;
	work_buf_size = cmp_cal_work_buf_size(&params, sizeof(src));
	TEST_ASSERT_LESS_THAN(work_buf_size, sizeof(work_buf));
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));

	return_code = compress_i16_in_i32_wrapper(&ctx, dst, sizeof(dst), src, sizeof(src));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_TOO_SMALL, return_code);
}


TEST_CASE(compress_u16_wrapper)
TEST_CASE(compress_i16_wrapper)
TEST_CASE(compress_i16_in_i32_wrapper)
void test_detect_src_size_change_using_model_preprocessing(compress_func_t compress_func)
{
	const int32_t src1[4] = { 0 };
	const int32_t src2[2] = { 0 };
	uint16_t work_buf[ARRAY_SIZE(src1) * sizeof(int16_t)];
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src1))];
	uint32_t return_code;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 10;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));
	TEST_ASSERT_CMP_SUCCESS(compress_func(&ctx, dst, sizeof(dst), src1, sizeof(src1)));

	return_code = compress_func(&ctx, dst, sizeof(dst), src2, sizeof(src2));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_MISMATCH, return_code);
}
