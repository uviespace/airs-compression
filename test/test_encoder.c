/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Encoder Tests
 */

#include <stdint.h>

#include <unity.h>
#include "test_common.h"

#include "../lib/cmp.h"
#include "../lib/cmp_errors.h"
#include "../lib/cmp_header.h"
#include "../lib/common/bitstream_writer.h"


void test_bitstream_write_nothing(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	DST_ALIGNED_U8 buffer[1] = { 0xFF };

	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bsw, buffer, sizeof(buffer)));

	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL(0, size);
}


void test_bitstream_write_single_bit_one(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	DST_ALIGNED_U8 buffer[1] = { 0xFF };

	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bsw, buffer, sizeof(buffer)));

	bitstream_add_bits32(&bsw, 1, 1);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL_UINT8(0x80, buffer[0]);
	TEST_ASSERT_EQUAL(1, size);
}


void test_bitstream_write_two_bits_zero_one(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	DST_ALIGNED_U8 buffer[1] = { 0xFF };

	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bsw, buffer, sizeof(buffer)));

	bitstream_add_bits32(&bsw, 0, 1);
	bitstream_add_bits32(&bsw, 1, 1);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL_UINT8(0x40, buffer[0]);
	TEST_ASSERT_EQUAL(1, size);
}


void test_bitstream_write_10bytes(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	DST_ALIGNED_U8 buffer[10];
	uint8_t expected_bs[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };

	memset(buffer, 0xFF, sizeof(buffer));

	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bsw, buffer, sizeof(buffer)));

	bitstream_add_bits32(&bsw, 0x0001, 16);
	bitstream_add_bits32(&bsw, 0x0203, 16);
	bitstream_add_bits32(&bsw, 0x0405, 16);
	bitstream_add_bits32(&bsw, 0x0607, 16);
	bitstream_add_bits32(&bsw, 0x0809, 16);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL(sizeof(expected_bs), size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_bs, buffer, sizeof(expected_bs));
}


void test_detect_bitstream_overflow(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	DST_ALIGNED_U8 buffer[1] = { 0xFF };

	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bsw, buffer, sizeof(buffer)));

	bitstream_add_bits32(&bsw, 0x1F, 9);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_TOO_SMALL, size);
}


static void run_encoder_test(enum cmp_encoder_type type, uint32_t encoder_param,
			     uint32_t encoder_outlier, const int16_t *input_data,
			     uint32_t input_size, const uint8_t *expected, uint32_t expected_size,
			     uint32_t expected_hdr_outlier)

{
	uint64_t output_buf[5]; /* enough for all tests */
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	memset(output_buf, 0xFF, sizeof(output_buf));
	params.primary_encoder_type = type;
	params.primary_encoder_param = encoder_param;
	params.primary_encoder_outlier = encoder_outlier;

	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf),
				       (const uint16_t *)input_data, input_size);

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + expected_size, output_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, cmp_hdr_get_cmp_data(output_buf), expected_size);
	{
		struct cmp_hdr expected_hdr = { 0 };

		expected_hdr.compressed_size = output_size;
		expected_hdr.original_size = input_size;
		expected_hdr.encoder_type = type;
		expected_hdr.encoder_param = encoder_param;
		expected_hdr.encoder_outlier = expected_hdr_outlier;
		TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
	}
}


void test_golomb_zero_param1_encodes_normal_values(void)
{
	const int16_t data[] = { -8, 7, -1, 0 };
	const uint8_t expected[] = { 0xFF, 0xFF, 0x7F, 0xFF, 0x68 };

	run_encoder_test(CMP_ENCODER_GOLOMB_ZERO, 1, 0, data, sizeof(data), expected,
			 sizeof(expected), 16);
}


void test_golomb_zero_param1_encodes_lowest_outlier(void)
{
	const int16_t data[] = { 8 };
	const uint8_t expected[] = { 0x00, 0x08, 0x00 };

	run_encoder_test(CMP_ENCODER_GOLOMB_ZERO, 1, 0, data, sizeof(data), expected,
			 sizeof(expected), 16);
}


void test_golomb_zero_param1_encodes_highest_outlier(void)
{
	const int16_t data[] = { INT16_MIN };
	const uint8_t expected[] = { 0x7F, 0xFF, 0x80 };

	run_encoder_test(CMP_ENCODER_GOLOMB_ZERO, 1, 0, data, sizeof(data), expected,
			 sizeof(expected), 16);
}


void test_golomb_zero_param10_encodes_normal_values(void)
{
	const int16_t data[] = { 82, 4, 0 };
	const uint8_t expected[] = { 0xFF, 0XFF, 0x57, 0x88 };

	run_encoder_test(CMP_ENCODER_GOLOMB_ZERO, 10, 0, data, sizeof(data), expected,
			 sizeof(expected), 165);
}


void test_golomb_zero_param10_encodes_lowest_outlier(void)
{
	const int16_t data[] = { -83 };
	const uint8_t expected[] = { 0x00, 0x0A, 0x50 };

	run_encoder_test(CMP_ENCODER_GOLOMB_ZERO, 10, 0, data, sizeof(data), expected,
			 sizeof(expected), 165);
}


void test_golomb_zero_param10_encodes_highest_outlier(void)
{
	const int16_t data[] = { INT16_MIN };
	const uint8_t expected[] = { 0x0F, 0xFF, 0xF0 };

	run_encoder_test(CMP_ENCODER_GOLOMB_ZERO, 10, 0, data, sizeof(data), expected,
			 sizeof(expected), 165);
}


void test_golomb_zero_param_max_encodes_normal_values(void)
{
	/* with this g_par we can encode all values, no outlier encoding */
	const int16_t data[] = { 0, INT16_MIN };
	const uint8_t expected[] = { 0x00, 0x01, 0x40, 0x00, 0x40 };

	run_encoder_test(CMP_ENCODER_GOLOMB_ZERO, UINT16_MAX, 0, data, sizeof(data), expected,
			 sizeof(expected), 0xFFFF0);
}


void test_golomb_multi_param1_encodes_normal_values(void)
{
	const int16_t data[] = { 0, 2 };
	const uint8_t expected[] = { 0x78 };

	run_encoder_test(CMP_ENCODER_GOLOMB_MULTI, 1, 5, data, sizeof(data), expected,
			 sizeof(expected), 5);
}


void test_golomb_multi_encodes_2bits_outliers(void)
{
	const int16_t data[] = { -3, 3, -4, 4 };
	const uint8_t expected[] = { 0xF8, 0xF9, 0xFA, 0xFB };

	run_encoder_test(CMP_ENCODER_GOLOMB_MULTI, 1, 5, data, sizeof(data), expected,
			 sizeof(expected), 5);
}


void test_golomb_multi_encodes_4bits_outliers(void)
{
	const int16_t data[] = { -5, 10 };
	const uint8_t expected[] = { 0xFC, 0x9F, 0xBC };

	run_encoder_test(CMP_ENCODER_GOLOMB_MULTI, 1, 5, data, sizeof(data), expected,
			 sizeof(expected), 5);
}


void test_golomb_multi_encodes_largest_16bits_outliers(void)
{
	const int16_t data[] = { INT16_MIN };
	const uint8_t expected[] = { 0xFF, 0xF7, 0xFF, 0xD0 };

	run_encoder_test(CMP_ENCODER_GOLOMB_MULTI, 1, 5, data, sizeof(data), expected,
			 sizeof(expected), 5);
}


void test_golomb_multi_param1_clamps_outlier_at_max_normal_value(void)
{
	const int16_t data[] = { -12 };
	const uint8_t expected[] = { 0xFF, 0xFF, 0xFE };

	run_encoder_test(CMP_ENCODER_GOLOMB_MULTI, 1, 42, data, sizeof(data), expected,
			 sizeof(expected), 24);
}


void test_golomb_multi_param1_clamps_outlier_at_minimum_outlier_value(void)
{
	const int16_t data[] = { 12 };
	const uint8_t expected[] = { 0xFF, 0xFF, 0xFF, 0x00 };

	run_encoder_test(CMP_ENCODER_GOLOMB_MULTI, 1, 42, data, sizeof(data), expected,
			 sizeof(expected), 24);
}


void test_golomb_multi_param1_clamps_outlier_at_max_outlier_value(void)
{
	const int16_t data[] = { INT16_MIN };
	const uint8_t expected[] = { 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xE7 };

	run_encoder_test(CMP_ENCODER_GOLOMB_MULTI, 1, 42, data, sizeof(data), expected,
			 sizeof(expected), 24);
}


void test_golomb_multi_param_max_encodes_zero_value(void)
{
	const int16_t data[] = { 0 };
	const uint8_t expected[] = { 0x00, 0x00 };

	run_encoder_test(CMP_ENCODER_GOLOMB_MULTI, UINT16_MAX, UINT32_MAX, data, sizeof(data),
			 expected, sizeof(expected), 0xFFFE9);
}


void test_golomb_multi_param_max_encodes_largest_value(void)
{
	const int16_t data[] = { INT16_MIN };
	const uint8_t expected[] = { 0x80, 0x00, 0x00 };

	run_encoder_test(CMP_ENCODER_GOLOMB_MULTI, UINT16_MAX, UINT32_MAX, data, sizeof(data),
			 expected, sizeof(expected), 0xFFFE9);
}


void test_use_secondary_encoder_for_second_pass(void)
{
	const uint16_t input_data[] = { 82, 4, 0 };
	const int8_t expected_primary[] = { 0, 82, 0, 4, 0, 0 };
	const uint8_t expected_secondary[] = { 0xFF, 0XFF, 0x57, 0x88 };
	DST_ALIGNED_U8 output_buf[CMP_HDR_SIZE + sizeof(expected_primary)];
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.secondary_iterations = 1;
	params.secondary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.secondary_encoder_param = 10;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	/* 1st pass */
	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input_data,
				       sizeof(input_data));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_primary), output_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_primary, cmp_hdr_get_cmp_data(output_buf),
				     sizeof(expected_primary));

	expected_hdr.compressed_size = CMP_HDR_SIZE + sizeof(expected_primary);
	expected_hdr.original_size = sizeof(input_data);
	expected_hdr.encoder_type = CMP_ENCODER_UNCOMPRESSED;
	expected_hdr.preprocess_param = params.secondary_iterations;
	TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);

	/* 2nd pass */
	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input_data,
				       sizeof(input_data));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_secondary), output_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_secondary, cmp_hdr_get_cmp_data(output_buf),
				     ARRAY_SIZE(expected_secondary));
	expected_hdr.sequence_number = 1;
	expected_hdr.compressed_size = CMP_HDR_SIZE + sizeof(expected_secondary);
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 10;
	expected_hdr.encoder_outlier = 165;
	TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
}
