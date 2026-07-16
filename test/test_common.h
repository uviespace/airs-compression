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

#include <stddef.h>
#include <string.h>
#include <unity.h>
#include "../lib/cmp.h"
#include "../lib/cmp_errors.h"
#include "../lib/common/compiler.h"
#include "../lib/common/header_private.h"

/*
 * Without ASan/MSan, overwrite stale and uninitialized data without changing
 * allocation layout. Include before arena.h to apply the test hooks.
 */
#if !CMP_ASAN_ENABLED && !CMP_MSAN_ENABLED && !defined(ARENA_POISON) && !defined(ARENA_UNPOISON)
enum { POISON = 0xBD, UNPOISON = 0xA0 };

static __inline void test_arena_poison(void *ptr, size_t size)
{
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_GREATER_THAN(0, size);
	memset(ptr, POISON, size);
}


static __inline void test_arena_unpoison(void *ptr, size_t size)
{
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_GREATER_THAN(0, size);
	memset(ptr, UNPOISON, size);
}

/* Destructive test hooks, not production memory-checker annotations. */
#  define ARENA_POISON(ptr, size)   test_arena_poison(ptr, size)
#  define ARENA_UNPOISON(ptr, size) test_arena_unpoison(ptr, size)

/* Keep the unsanitized production layout: no extra gaps or alignment. */
#  ifndef ARENA_RED_ZONE_SIZE
#    define ARENA_RED_ZONE_SIZE 0
#  endif
#endif

#include "../lib/common/arena.h"

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
 * @brief Retrieve pointer to compressed data following the header
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
 * @brief Clears and returns the test arena for memory allocation
 *
 * The arena's state is completely reset on each call, providing a fresh scratch
 * space for the caller. Consequently, any data allocated from the arena
 * in previous calls becomes invalid.
 *
 * @warning Call it only once for a test. A reset invalidates every prior
 *          allocation from the shared test arena.
 *
 * @returns pointer to the cleared arena instance
 */
struct arena *clear_test_arena(void);


struct test_src {
	const void *data;
	uint32_t size;
	uint32_t packed_size;
};

/**
 * @brief Converts canonical test samples to a compressor-specific format
 *
 * The input samples use a common int32_t representation and are converted to
 * the representation selected by @p dtype. The resulting data is allocated
 * from arena @p a.
 *
 * @param a		arena used for the converted source data
 * @param dtype		data type to which the samples are converted
 * @param samples	input samples represented as int32_t values
 * @param sample_count	number of input samples
 *
 * @returns converted source data, its storage size, and its packed size; the
 *	data remains valid until the arena is cleared or goes out of scope
 */

struct test_src make_test_src(struct arena *a, enum cmp_type dtype, const int32_t *samples,
			      uint32_t sample_count);


/** @brief Test fixture bundling compression function with its metadata */
struct t_fixture {
	uint32_t (*compress)(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
			     const void *src, uint32_t src_size);
	enum cmp_type dtype;
};


/*
 * extern declarations needed here so the test runner can find the arguments
 * passed to the parametrized tests.
 */
extern const struct t_fixture t_fix_u16;
extern const struct t_fixture t_fix_i16;
extern const struct t_fixture t_fix_i16_in_i32;
extern const struct t_fixture t_fix_raw12;

extern const uint8_t expected_uncompressed_16bit[12];
extern const uint8_t expected_uncompressed_raw12[9];

extern const int32_t t_diff[12];
extern const int16_t t_exp_diff[12];
extern const uint32_t t_diff_raw12_count;

extern const int32_t t_iwt1[1];
extern const int16_t t_exp_iwt1[1];
extern const int32_t t_iwt2[2];
extern const int16_t t_exp_iwt2[2];
extern const int32_t t_iwt5[5];
extern const int16_t t_exp_iwt5[5];
extern const int32_t t_iwt7[7];
extern const int16_t t_exp_iwt7[7];
extern const int32_t t_iwt8[8];
extern const int16_t t_exp_iwt8[8];

extern const int32_t t_model1_u16[7];
extern const int32_t t_model2_u16[7];
extern const int32_t t_model3_u16[7];
extern const int16_t t_exp_model_u16[7];
extern const uint32_t t_model_raw12_count;
extern const int32_t t_model1_i16[7];
extern const int32_t t_model2_i16[7];
extern const int32_t t_model3_i16[7];
extern const int16_t t_exp_model_i16[7];

#endif /* TEST_COMMON_H */
