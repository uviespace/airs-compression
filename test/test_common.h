/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Utilities for testing the compression library
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <unity.h>
#include "../lib/cmp_errors.h"
#include "../lib/common/compiler.h"
#include "../lib/common/header_private.h"

#define ARRAY_AND_SIZE(arr) (arr), sizeof(arr)

/** uint8_t type with the required compression destination buffer alignment */
#define DST_ALIGNED_U8 ALIGNED_TYPE(CMP_DST_ALIGNMENT, uint8_t)


void assert_equal_cmp_error_internal(enum cmp_error expected_error, uint32_t cmp_ret_code,
				     int line);


/**
 * @brief Validates successful compression function call
 *
 * @param cmp_ret_code	return code from compression library function
 */
#define TEST_ASSERT_CMP_SUCCESS(cmp_ret_code)                            \
	do {                                                             \
		uint32_t _cmp_ret = (cmp_ret_code);                      \
		TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_NO_ERROR, _cmp_ret); \
	} while (0)


/**
 * @brief Checks compression function failure
 *
 * @param cmp_ret_code	return code from compression library function
 */
#define TEST_ASSERT_CMP_FAILURE(cmp_ret_code) TEST_ASSERT_TRUE(cmp_is_error(cmp_ret_code))


/**
 * @brief Asserts compression error code equality
 *
 * @param expected_CMP_ERROR	expected error code
 * @param cmp_ret_code		compression library return code
 */

#define TEST_ASSERT_EQUAL_CMP_ERROR(expected_CMP_ERROR, cmp_ret_code) \
	assert_equal_cmp_error_internal(expected_CMP_ERROR, cmp_ret_code, __LINE__)


/**
 * @brief Asserts that a compressed data header matches the expected header
 *
 * The model_id is ignored.
 *
 * @param compressed_data	pointer to the compressed data buffer
 * @param size			size of the compressed data buffer
 * @param expected_hdr		constant pointer to the expected cmp_hdr structure
 */

#define TEST_ASSERT_CMP_HDR(compressed_data, size, expected_hdr)                                   \
	do {                                                                                       \
		struct cmp_hdr assert_hdr;                                                         \
		TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(compressed_data, size, &assert_hdr));  \
		expected_hdr.version = CMP_VERSION_NUMBER;       /* always expected */             \
		assert_hdr.identifier = expected_hdr.identifier; /* ignore this field */           \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.version, assert_hdr.version,                \
					  "header version ID mismatch");                           \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.compressed_size,                            \
					  assert_hdr.compressed_size,                              \
					  "header compressed data size mismatch");                 \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.original_size, assert_hdr.original_size,    \
					  "header original size mismatch");                        \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.checksum, assert_hdr.checksum,              \
					  "header checksum mismatch");                             \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.sequence_number,                            \
					  assert_hdr.sequence_number,                              \
					  "header sequence number mismatch");                      \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.preprocessing, assert_hdr.preprocessing,    \
					  "header preprocessing mismatch");                        \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.encoder_type, assert_hdr.encoder_type,      \
					  "header encoder mismatch");                              \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.encoder_param, assert_hdr.encoder_param,    \
					  "header encoder parameter mismatch");                    \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.encoder_outlier,                            \
					  assert_hdr.encoder_outlier,                              \
					  "header outlier parameter mismatch");                    \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.original_dtype, assert_hdr.original_dtype,  \
					  "header original data type mismatch");                   \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.preprocess_param,                           \
					  assert_hdr.preprocess_param,                             \
					  "header preprocess param mismatch");                     \
		TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&expected_hdr, &assert_hdr, sizeof(expected_hdr), \
						 "header mismatch");                               \
	} while (0)


/**
 * @brief retrieve pointer to compressed data following the header
 *
 * @warning Assumes the compressed data block starts with a valid header.
 *
 * @param header	pointer to the start of the compressed data (header)
 *
 * @returns a pointer to the first byte of compressed data after the header
 *
 */

const void *cmp_hdr_get_cmp_data(const void *header);


/**
 * Wrapper around malloc() that asserts the allocation is successful.  If
 * allocation fails, the test will fail with an assertion.
 */

void *t_malloc(size_t size);


struct test_env {
	void *dst;
	void *work;
	struct cmp_context ctx;
	uint32_t dst_cap;
};

struct test_env *make_env(struct cmp_params *params, uint32_t src_len);
void free_env(struct test_env *e);


/**
 * @brief Test fixture bundling compression function with its metadata
 */
struct cmp_test_fixture {
	uint32_t (*compress)(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
			     const void *src, uint32_t src_size);
	enum cmp_type dtype;
};

extern const struct cmp_test_fixture cmp_fixture_u16;
extern const struct cmp_test_fixture cmp_fixture_i16;
extern const struct cmp_test_fixture cmp_fixture_i16_in_i32;


extern const uint16_t test_dummy_u16[2];
extern const int16_t test_dummy_i16[2];
extern const int32_t test_dummy_i16_in_i32[2];

extern const uint16_t model_input1_u16[5];
extern const uint16_t model_input2_u16[5];
extern const uint16_t model_input3_u16[5];
extern const int16_t expec_output_u16[5];

extern const int16_t model_input1_i16[7];
extern const int16_t model_input2_i16[7];
extern const int16_t model_input3_i16[7];
extern const int16_t expected_out_i16[7];

extern const int32_t model_input1_i16_in_i32[7];
extern const int32_t model_input2_i16_in_i32[7];
extern const int32_t model_input3_i16_in_i32[7];

extern const int16_t g_iwt_input_1[1];
extern const int32_t g_iwt_input_1_i32[1];
extern const int16_t g_iwt_exp_out_1[1];
extern const int16_t g_iwt_input_2[2];
extern const int32_t g_iwt_input_2_i32[2];
extern const int16_t g_iwt_exp_out_2[2];
extern const int16_t g_iwt_input_5[5];
extern const int32_t g_iwt_input_5_i32[5];
extern const int16_t g_iwt_exp_out_5[5];
extern const int16_t g_iwt_input_7[7];
extern const int32_t g_iwt_input_7_i32[7];
extern const int16_t g_iwt_exp_out_7[7];
extern const int16_t g_iwt_input_8[8];
extern const int32_t g_iwt_input_8_i32[8];
extern const int16_t g_iwt_exp_out_8[8];

extern const uint16_t test_diff_u16[8];
extern const int16_t test_diff_i16[8];
extern const int32_t test_diff_i16_in_i32[8];

#endif /* TEST_COMMON_H */
