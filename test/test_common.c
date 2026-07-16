/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Utilities for testing the compression library
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <unity.h>
#include <unity_internals.h>


#include "test_common.h"
#include "../lib/cmp_errors.h"
#include "../lib/cmp_header.h"
#include "../lib/common/arena.h"


struct test_src make_test_src(struct arena *a, enum cmp_type dtype, const int32_t *samples,
			      uint32_t sample_count)
{
	struct test_src r = { 0 };
	uint32_t i;

	TEST_ASSERT_NOT_NULL(a);
	TEST_ASSERT_NOT_NULL(samples);
	TEST_ASSERT_GREATER_THAN_UINT32(0, sample_count);

	switch (dtype) {
	case CMP_I16: {
		int16_t *data = ARENA_NEW_ARRAY(a, (ptrdiff_t)sample_count, int16_t);

		for (i = 0; i < sample_count; i++) {
			TEST_ASSERT_GREATER_OR_EQUAL_INT32(INT16_MIN, samples[i]);
			TEST_ASSERT_LESS_OR_EQUAL_INT32(INT16_MAX, samples[i]);
			data[i] = (int16_t)samples[i];
		}
		r.data = data;
		r.size = sample_count * sizeof(*data);
		break;
	}
	case CMP_U16: {
		uint16_t *data = ARENA_NEW_ARRAY(a, (ptrdiff_t)sample_count, uint16_t);

		for (i = 0; i < sample_count; i++) {
			/* we allow negative values in range and simply cast to unsigned */
			TEST_ASSERT_GREATER_OR_EQUAL_INT32(INT16_MIN, samples[i]);
			TEST_ASSERT_LESS_OR_EQUAL_INT32(UINT16_MAX, samples[i]);
			data[i] = (uint16_t)samples[i];
		}
		r.data = data;
		r.size = sample_count * sizeof(*data);
		break;
	}
	case CMP_I16_IN_I32: {
		int32_t *data = ARENA_NEW_ARRAY(a, (ptrdiff_t)sample_count, int32_t);

		for (i = 0; i < sample_count; i++) {
			TEST_ASSERT_GREATER_OR_EQUAL_INT32(INT16_MIN, samples[i]);
			TEST_ASSERT_LESS_OR_EQUAL_INT32(INT16_MAX, samples[i]);
			data[i] = samples[i];
		}
		r.data = data;
		r.size = sample_count * sizeof(*data);
		break;
	}
	case CMP_RAW12: {
		uint8_t *data;

		r.size = (uint32_t)((((uint64_t)sample_count * 3) + 1) / 2);
		data = ARENA_NEW_ARRAY(a, (ptrdiff_t)r.size, uint8_t);
		r.data = data;
		for (i = 0; i < (sample_count & ~1U); i += 2) {
			int32_t s1 = samples[i];
			int32_t s2 = samples[i + 1];

			/* we allow negative values in range and simply cast to unsigned */
			TEST_ASSERT_GREATER_OR_EQUAL_INT32(-0x800, s1);
			TEST_ASSERT_LESS_OR_EQUAL_INT32(0xFFF, s1);
			TEST_ASSERT_GREATER_OR_EQUAL_INT32(-0x800, s2);
			TEST_ASSERT_LESS_OR_EQUAL_INT32(0xFFF, s2);
			*data++ = (uint8_t)(s1 & 0x00FF);
			*data++ = (uint8_t)(((s2 & 0x000F) << 4) | ((s1 & 0x0F00) >> 8));
			*data++ = (uint8_t)((s2 & 0xFF0) >> 4);
		}
		if (sample_count & 1U) {
			int32_t s1 = samples[sample_count - 1];

			TEST_ASSERT_GREATER_OR_EQUAL_INT32(-0x800, s1);
			TEST_ASSERT_LESS_OR_EQUAL_INT32(0xFFF, s1);
			*data++ = (uint8_t)(s1 & 0x00FF);
			*data++ = (uint8_t)((s1 & 0x0F00) >> 8);
		}
		break;
	}
	default:
		TEST_FAIL_MESSAGE("Unsupported sample type");
		return r;
	}

	r.packed_size = dtype == CMP_I16_IN_I32 ? sample_count * sizeof(uint16_t) : r.size;
	return r;
}


const void *cmp_hdr_get_cmp_data(const void *header)
{
	TEST_ASSERT_NOT_NULL(header);
	return (const uint8_t *)header + CMP_HDR_SIZE;
}


/**
 * @brief Converts compression error enum to string
 *
 * @param error	compression error code
 *
 * @returns error code string
 */

static const char *cmp_error_enum_to_str(enum cmp_error error)
{
	switch (error) {
	case CMP_ERR_NO_ERROR:
		return "CMP_ERR_NO_ERROR";
	case CMP_ERR_GENERIC:
		return "CMP_ERR_GENERIC";
	case CMP_ERR_PARAMS_INVALID:
		return "CMP_ERR_PARAMS_INVALID";
	case CMP_ERR_CONTEXT_INVALID:
		return "CMP_ERR_CONTEXT_INVALID";
	case CMP_ERR_WORK_BUF_NULL:
		return "CMP_ERR_WORK_BUF_NULL";
	case CMP_ERR_WORK_BUF_TOO_SMALL:
		return "CMP_ERR_WORK_BUF_TOO_SMALL";
	case CMP_ERR_WORK_BUF_UNALIGNED:
		return "CMP_ERR_WORK_BUF_UNALIGNED";
	case CMP_ERR_DST_NULL:
		return "CMP_ERR_DST_NULL";
	case CMP_ERR_DST_UNALIGNED:
		return "CMP_ERR_DST_UNALIGNED";
	case CMP_ERR_SRC_NULL:
		return "CMP_ERR_SRC_NULL";
	case CMP_ERR_SRC_SIZE_WRONG:
		return "CMP_ERR_SRC_SIZE_WRONG";
	case CMP_ERR_DST_TOO_SMALL:
		return "CMP_ERR_DST_TOO_SMALL";
	case CMP_ERR_SRC_MISMATCH:
		return "CMP_ERR_SRC_MISMATCH";
	case CMP_ERR_INT_HDR:
		return "CMP_ERR_INT_HDR";
	case CMP_ERR_INT_ENCODER:
		return "CMP_ERR_INT_ENCODER";
	case CMP_ERR_INT_BITSTREAM:
		return "CMP_ERR_INT_BITSTREAM";
	case CMP_ERR_HDR_CMP_SIZE_TOO_LARGE:
		return "CMP_ERR_HDR_CMP_SIZE_TOO_LARGE";
	case CMP_ERR_HDR_ORIGINAL_TOO_LARGE:
		return "CMP_ERR_HDR_ORIGINAL_TOO_LARGE";
	case CMP_ERR_HDR_UNSUPPORTED:
		return "CMP_ERR_HDR_UNSUPPORTED";
	case CMP_ERR_MAX_CODE:
	default:
		TEST_FAIL_MESSAGE("Missing error name");
		return "Unknown error";
	}
}


/**
 * @brief Generates a descriptive error message
 *
 * @param expected	expected error code
 * @param actual	actual error code
 *
 * @returns formatted error message string
 */

static const char *gen_cmp_error_message(enum cmp_error expected, enum cmp_error actual)
{
	enum { CMP_TEST_MESSAGE_BUF_SIZE = 128 };
	static char message[CMP_TEST_MESSAGE_BUF_SIZE];

	snprintf(message, sizeof(message), "Expected %s Was %s.", cmp_error_enum_to_str(expected),
		 cmp_error_enum_to_str(actual));
	return message;
}


/**
 * @brief Asserts compression error code equality
 *
 * @param expected_error	expected error code
 * @param cmp_ret_code		compression library return code
 */

void assert_equal_cmp_error_internal(enum cmp_error expected_error, uint32_t cmp_ret_code, int line)
{
	enum cmp_error actual_error = cmp_get_error_code(cmp_ret_code);
	const char *message = gen_cmp_error_message(expected_error, actual_error);

	UNITY_TEST_ASSERT_EQUAL_INT(expected_error, actual_error, line, message);
}


/* Test-specific OOM handler that fails the test instead of exiting */
static void test_oom_handler(void)
{
	TEST_FAIL_MESSAGE("Arena allocation failed: out of memory");
}


struct arena *clear_test_arena(void)
{
	static uint8_t mem[1 << 10];
	static struct arena a;

	arena_set_oom_handler(test_oom_handler);

	/* Reset the arena and poison mem buffer via the configured poison hook. */
	a = arena_init(mem, sizeof(mem));
	return &a;
}


static uint32_t compress_u16_wrapper(struct cmp_context *ctx, void *dst, uint32_t cap,
				     const void *src, uint32_t src_size)
{
	return cmp_compress_u16(ctx, dst, cap, src, src_size);
}


static uint32_t compress_i16_wrapper(struct cmp_context *ctx, void *dst, uint32_t cap,
				     const void *src, uint32_t src_size)
{
	return cmp_compress_i16(ctx, dst, cap, src, src_size);
}


static uint32_t compress_i16_in_i32_wrapper(struct cmp_context *ctx, void *dst, uint32_t cap,
					    const void *src, uint32_t src_size)
{
	return cmp_compress_i16_in_i32(ctx, dst, cap, src, src_size);
}


static uint32_t compress_raw12_wrapper(struct cmp_context *ctx, void *dst, uint32_t cap,
				       const void *src, uint32_t src_size)
{
	return cmp_compress_raw12(ctx, dst, cap, src, src_size);
}


const struct t_fixture t_fix_u16 = { compress_u16_wrapper, CMP_U16 };
const struct t_fixture t_fix_i16 = { compress_i16_wrapper, CMP_I16 };
const struct t_fixture t_fix_i16_in_i32 = { compress_i16_in_i32_wrapper, CMP_I16_IN_I32 };
const struct t_fixture t_fix_raw12 = { compress_raw12_wrapper, CMP_RAW12 };
