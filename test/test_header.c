/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief data compression header tests
 */

#include <stdint.h>
#include <string.h>

#include <unity.h>
#include "test_common.h"

#include "../lib/cmp.h"
#include "../lib/cmp_header.h"
#include "../lib/cmp_errors.h"
#include "../lib/common/bitstream_writer.h"


#define MAX_VALUE(max_bits) ((1ULL << (max_bits)) - 1)

void test_serialize_compression_header(void)
{
	DST_ALIGNED_U8 buf[CMP_HDR_SIZE];
	struct cmp_hdr hdr = { 0 };
	uint32_t hdr_size;
	int i;
	struct bitstream_writer bs;

	memset(buf, 0xAB, sizeof(buf));
	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bs, buf, sizeof(buf)));
	hdr.version = 0x0001;
	hdr.compressed_size = 0x020304;
	hdr.original_size = 0x050607;
	hdr.checksum = 0x08090A0B;
	hdr.identifier = 0x0C0D0E0F;
	hdr.sequence_number = 0x10;
	hdr.preprocessing = 0x0;
	hdr.encoder_type = 0x2;
	hdr.original_dtype = 0x1;
	hdr.encoder_param = 0x1213;
	hdr.encoder_outlier = 0x141516;
	hdr.preprocess_param = 0x17;

	hdr_size = cmp_hdr_serialize(&bs, &hdr);

	TEST_ASSERT_CMP_SUCCESS(hdr_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE, hdr_size);
	for (i = 0; i < CMP_HDR_SIZE; i++)
		TEST_ASSERT_EQUAL_HEX8(i, buf[i]);
}


void test_serialize_compression_header_with_maximum_values(void)
{
	DST_ALIGNED_U8 buf[CMP_HDR_SIZE];
	struct cmp_hdr hdr = { 0 };
	uint32_t hdr_size;
	struct bitstream_writer bs;

	memset(buf, 0xAB, sizeof(buf));
	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bs, buf, sizeof(buf)));
	hdr.version = MAX_VALUE(CMP_HDR_BITS_VERSION);
	hdr.compressed_size = MAX_VALUE(CMP_HDR_BITS_COMPRESSED_SIZE);
	hdr.original_size = MAX_VALUE(CMP_HDR_BITS_ORIGINAL_SIZE);
	hdr.checksum = MAX_VALUE(CMP_HDR_BITS_CHECKSUM);
	hdr.identifier = MAX_VALUE(CMP_HDR_BITS_IDENTIFIER);
	hdr.sequence_number = MAX_VALUE(CMP_HDR_BITS_SEQUENCE_NUMBER);
	hdr.preprocessing = MAX_VALUE(CMP_HDR_BITS_PREPROCESSING);
	hdr.encoder_type = MAX_VALUE(CMP_HDR_BITS_ENCODER_TYPE);
	hdr.encoder_param = MAX_VALUE(CMP_HDR_BITS_ENCODER_PARAM);
	hdr.encoder_outlier = MAX_VALUE(CMP_HDR_BITS_ENCODER_OUTLIER);
	hdr.original_dtype = MAX_VALUE(CMP_HDR_BITS_ORIGINAL_DTYPE);
	hdr.preprocess_param = MAX_VALUE(CMP_HDR_BITS_PREPROCESS_PARAM);

	hdr_size = cmp_hdr_serialize(&bs, &hdr);

	TEST_ASSERT_CMP_SUCCESS(hdr_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE, hdr_size);
	TEST_ASSERT_EACH_EQUAL_HEX8(0xFF, buf, CMP_HDR_SIZE);
}


void test_deserialize_compression_header(void)
{
	DST_ALIGNED_U8 buf[CMP_HDR_SIZE];
	struct cmp_hdr hdr = { 0 };
	struct cmp_hdr expected_hdr;
	uint32_t hdr_size;
	int i;

	expected_hdr.version = 0x0001;
	expected_hdr.compressed_size = 0x020304;
	expected_hdr.original_size = 0x050607;
	expected_hdr.checksum = 0x08090A0B;
	expected_hdr.identifier = 0x0C0D0E0F;
	expected_hdr.sequence_number = 0x10;
	expected_hdr.preprocessing = 0x0;
	expected_hdr.encoder_type = 0x2;
	expected_hdr.original_dtype = 0x1;
	expected_hdr.encoder_param = 0x1213;
	expected_hdr.encoder_outlier = 0x141516;
	expected_hdr.preprocess_param = 0x17;
	for (i = 0; i < CMP_HDR_SIZE; i++)
		buf[i] = (uint8_t)i;

	hdr_size = cmp_hdr_deserialize(buf, sizeof(buf), &hdr);

	TEST_ASSERT_CMP_SUCCESS(hdr_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE, hdr_size);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.version, hdr.version);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.compressed_size, hdr.compressed_size);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.original_size, hdr.original_size);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.checksum, hdr.checksum);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.identifier, hdr.identifier);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.sequence_number, hdr.sequence_number);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.preprocessing, hdr.preprocessing);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.encoder_type, hdr.encoder_type);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.encoder_param, hdr.encoder_param);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.encoder_outlier, hdr.encoder_outlier);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.original_dtype, hdr.original_dtype);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.preprocess_param, hdr.preprocess_param);
}

void test_deserialize_compression_header_with_maximum_values(void)
{
	DST_ALIGNED_U8 buf[CMP_HDR_SIZE];
	struct cmp_hdr hdr = { 0 };
	struct cmp_hdr expected_hdr;
	uint32_t hdr_size;

	expected_hdr.version = MAX_VALUE(CMP_HDR_BITS_VERSION);
	expected_hdr.compressed_size = MAX_VALUE(CMP_HDR_BITS_COMPRESSED_SIZE);
	expected_hdr.original_size = MAX_VALUE(CMP_HDR_BITS_ORIGINAL_SIZE);
	expected_hdr.checksum = MAX_VALUE(CMP_HDR_BITS_CHECKSUM);
	expected_hdr.identifier = MAX_VALUE(CMP_HDR_BITS_IDENTIFIER);
	expected_hdr.sequence_number = MAX_VALUE(CMP_HDR_BITS_SEQUENCE_NUMBER);
	expected_hdr.preprocessing = MAX_VALUE(CMP_HDR_BITS_PREPROCESSING);
	expected_hdr.encoder_type = MAX_VALUE(CMP_HDR_BITS_ENCODER_TYPE);
	expected_hdr.encoder_param = MAX_VALUE(CMP_HDR_BITS_ENCODER_PARAM);
	expected_hdr.encoder_outlier = MAX_VALUE(CMP_HDR_BITS_ENCODER_OUTLIER);
	expected_hdr.original_dtype = MAX_VALUE(CMP_HDR_BITS_ORIGINAL_DTYPE);
	expected_hdr.preprocess_param = MAX_VALUE(CMP_HDR_BITS_PREPROCESS_PARAM);
	memset(buf, 0xFF, sizeof(buf));

	hdr_size = cmp_hdr_deserialize(buf, sizeof(buf), &hdr);

	TEST_ASSERT_CMP_SUCCESS(hdr_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE, hdr_size);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.version, hdr.version);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.compressed_size, hdr.compressed_size);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.original_size, hdr.original_size);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.checksum, hdr.checksum);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.identifier, hdr.identifier);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.sequence_number, hdr.sequence_number);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.preprocessing, hdr.preprocessing);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.encoder_type, hdr.encoder_type);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.encoder_param, hdr.encoder_param);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.encoder_outlier, hdr.encoder_outlier);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.original_dtype, hdr.original_dtype);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.preprocess_param, hdr.preprocess_param);
}


void test_hdr_serialize_detects_when_a_field_is_too_big(void)
{
#define TEST_HDR_FIELD_TOO_BIG(field, bits_for_field, exp_error)                       \
	do {                                                                           \
		DST_ALIGNED_U8 buf[CMP_HDR_SIZE];                                      \
		struct cmp_hdr hdr = { 0 };                                            \
		uint32_t hdr_size;                                                     \
		struct bitstream_writer bs;                                            \
		TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bs, buf, sizeof(buf))); \
		TEST_ASSERT(bits_for_field < bitsizeof(uint32_t));                     \
		TEST_ASSERT(bits_for_field > 0);                                       \
		hdr.field = 1UL << bits_for_field;                                     \
		hdr_size = cmp_hdr_serialize(&bs, &hdr);                               \
		TEST_ASSERT_EQUAL_CMP_ERROR(exp_error, hdr_size);                      \
		/* now it should work */                                               \
		TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bs, buf, sizeof(buf))); \
		hdr.field--;                                                           \
		hdr_size = cmp_hdr_serialize(&bs, &hdr);                               \
		TEST_ASSERT_CMP_SUCCESS(hdr_size);                                     \
	} while (0)

	/* TEST_HDR_FIELD_TOO_BIG(version, CMP_HDR_BITS_VERSION, CMP_ERR_NO_ERROR); */
	TEST_HDR_FIELD_TOO_BIG(compressed_size, CMP_HDR_BITS_COMPRESSED_SIZE,
			       CMP_ERR_HDR_CMP_SIZE_TOO_LARGE);
	TEST_HDR_FIELD_TOO_BIG(original_size, CMP_HDR_BITS_ORIGINAL_SIZE,
			       CMP_ERR_HDR_ORIGINAL_TOO_LARGE);
	/* TEST_HDR_FIELD_TOO_BIG(checksum, CMP_HDR_BITS_CHECKSUM, CMP_ERR_INT_BITSTREAM); */
	/* TEST_HDR_FIELD_TOO_BIG(identifier, CMP_HDR_BITS_IDENTIFIER, CMP_ERR_INT_BITSTREAM); */
	/* TEST_HDR_FIELD_TOO_BIG(sequence_number, CMP_HDR_BITS_SEQUENCE_NUMBER, CMP_ERR_INT_BITSTREAM); */

	/* I do not know the size of a enum this test may be flaky */
	TEST_HDR_FIELD_TOO_BIG(preprocessing, CMP_HDR_BITS_PREPROCESSING, CMP_ERR_INT_BITSTREAM);
	TEST_HDR_FIELD_TOO_BIG(encoder_type, CMP_HDR_BITS_ENCODER_TYPE, CMP_ERR_INT_BITSTREAM);

	TEST_HDR_FIELD_TOO_BIG(encoder_param, CMP_HDR_BITS_ENCODER_PARAM, CMP_ERR_INT_BITSTREAM);
	TEST_HDR_FIELD_TOO_BIG(encoder_outlier, CMP_HDR_BITS_ENCODER_OUTLIER,
			       CMP_ERR_INT_BITSTREAM);
	TEST_HDR_FIELD_TOO_BIG(original_dtype, CMP_HDR_BITS_ORIGINAL_DTYPE, CMP_ERR_INT_BITSTREAM);
	TEST_HDR_FIELD_TOO_BIG(preprocess_param, CMP_HDR_BITS_PREPROCESS_PARAM, CMP_ERR_INT_BITSTREAM);
#undef TEST_HDR_FIELD_TOO_BIG
}


void test_detect_null_hdr_during_serialize(void)
{
	DST_ALIGNED_U8 buf[CMP_HDR_SIZE];
	struct bitstream_writer bs;
	uint32_t ret;

	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bs, buf, sizeof(buf)));

	ret = cmp_hdr_serialize(&bs, NULL);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_INT_HDR, ret);
}


void test_detect_null_hdr_during_deserialize(void)
{
	DST_ALIGNED_U8 buf[CMP_HDR_SIZE] = { 0 };
	uint32_t ret;

	ret = cmp_hdr_deserialize(buf, sizeof(buf), NULL);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_INT_HDR, ret);
}

#undef MAX_VALUE
