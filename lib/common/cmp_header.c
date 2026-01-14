/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Compression header implementation
 */


#include <stdint.h>
#include <string.h>

#include "../cmp_header.h"
#include "header_private.h"
#include "err_private.h"
#include "sample_reader.h"
#include "bitstream_writer.h"

#define XXH_INLINE_ALL
#define XXH_STATIC_LINKING_ONLY
#define XXH_NO_STDLIB
#include "xxhash.h"


uint32_t cmp_hdr_serialize(struct bitstream_writer *bs, const struct cmp_hdr *hdr)
{
	uint32_t start_size, end_size;

	if (!hdr)
		return CMP_ERROR(INT_HDR);

	if (hdr->compressed_size > CMP_HDR_MAX_COMPRESSED_SIZE)
		return CMP_ERROR(HDR_CMP_SIZE_TOO_LARGE);

	if (hdr->original_size > CMP_HDR_MAX_ORIGINAL_SIZE)
		return CMP_ERROR(HDR_ORIGINAL_TOO_LARGE);

	start_size = bitstream_size(bs);
	if (cmp_is_error_int(start_size))
		return start_size;

	bitstream_add_bits32(bs, hdr->version, CMP_HDR_BITS_VERSION);
	bitstream_add_bits32(bs, hdr->compressed_size, CMP_HDR_BITS_COMPRESSED_SIZE);
	bitstream_add_bits32(bs, hdr->original_size, CMP_HDR_BITS_ORIGINAL_SIZE);

	bitstream_add_bits32(bs, hdr->checksum, CMP_HDR_BITS_CHECKSUM);
	bitstream_add_bits32(bs, hdr->identifier, CMP_HDR_BITS_IDENTIFIER);

	bitstream_add_bits32(bs, hdr->sequence_number, CMP_HDR_BITS_SEQUENCE_NUMBER);
	bitstream_add_bits32(bs, hdr->preprocessing, CMP_HDR_BITS_PREPROCESSING);
	bitstream_add_bits32(bs, hdr->encoder_type, CMP_HDR_BITS_ENCODER_TYPE);
	bitstream_add_bits32(bs, hdr->original_dtype, CMP_HDR_BITS_ORIGINAL_DTYPE);
	bitstream_add_bits32(bs, hdr->encoder_param, CMP_HDR_BITS_ENCODER_PARAM);
	bitstream_add_bits32(bs, hdr->encoder_outlier, CMP_HDR_BITS_ENCODER_OUTLIER);
	bitstream_add_bits32(bs, hdr->preprocess_param, CMP_HDR_BITS_PREPROCESS_PARAM);

	end_size = bitstream_flush(bs);
	if (cmp_is_error_int(end_size))
		return end_size;

	return end_size - start_size;
}


/* clang-format off */
static uint16_t extract_u16be(const uint8_t *buf)
{
	return (uint16_t)(buf[0] << 8) |
	       (uint16_t)(buf[1] << 0);
}


static uint32_t extract_u24be(const uint8_t *buf)
{
	return (uint32_t)buf[0] << 16 |
	       (uint32_t)buf[1] << 8  |
	       (uint32_t)buf[2] << 0;
}


static uint32_t extract_u32be(const uint8_t *buf)
{
	return (uint32_t)buf[0] << 24 |
	       (uint32_t)buf[1] << 16 |
	       (uint32_t)buf[2] << 8  |
	       (uint32_t)buf[3] << 0;
}
/* clang-format on */


uint32_t cmp_hdr_deserialize(const void *src, uint32_t src_size, struct cmp_hdr *hdr)
{
	const uint8_t *start = src;
	uint8_t prepros_enc_type_odt;

	if (!hdr)
		return CMP_ERROR(INT_HDR);
	if (!src)
		return CMP_ERROR(INT_HDR);
	if (src_size < CMP_HDR_SIZE)
		return CMP_ERROR(INT_HDR);
	memset(hdr, 0x00, sizeof(*hdr));

	hdr->version = extract_u16be(start + CMP_HDR_OFFSET_VERSION);
	hdr->compressed_size = extract_u24be(start + CMP_HDR_OFFSET_COMPRESSED_SIZE);
	hdr->original_size = extract_u24be(start + CMP_HDR_OFFSET_ORIGINAL_SIZE);

	hdr->checksum = extract_u32be(start + CMP_HDR_OFFSET_CHECKSUM);
	hdr->identifier = extract_u32be(start + CMP_HDR_OFFSET_IDENTIFIER);

	hdr->sequence_number = start[CMP_HDR_OFFSET_SEQUENCE_NUMBER];

	prepros_enc_type_odt = start[CMP_HDR_OFFSET_PED_FIELDS];
	hdr->preprocessing = (prepros_enc_type_odt >> 5) & 0x7;
	hdr->encoder_type = (prepros_enc_type_odt >> 3) & 0x3;
	hdr->original_dtype = prepros_enc_type_odt & 0x7;

	hdr->encoder_param = extract_u16be(start + CMP_HDR_OFFSET_ENCODER_PARAM);
	hdr->encoder_outlier = extract_u24be(start + CMP_HDR_OFFSET_OUTLIER_PARAM);
	hdr->preprocess_param = start[CMP_HDR_OFFSET_PREPROCESS_PARAM];

	return CMP_HDR_SIZE;
}


uint32_t cmp_hdr_checksum_int(const struct sample_desc *desc)
{
	uint32_t i;
	XXH32_state_t state;

	/*
	 * Fast path: on big-endian systems with contiguous data, we can hash
	 * directly without byte swapping.
	 */
	if (!XXH_CPU_LITTLE_ENDIAN && (desc->type == CMP_I16 || desc->type == CMP_U16))
		return XXH32(desc->data, desc->num_samples * sizeof(uint16_t), CHECKSUM_SEED);

	/*
	 * Slow path: convert each sample to big-endian for consistent checksums
	 * across architectures.
	 */
	(void)XXH32_reset(&state, CHECKSUM_SEED);
	for (i = 0; i < desc->num_samples; i++) {
		uint16_t value = (uint16_t)sample_read_i16(desc, i);

		if (XXH_CPU_LITTLE_ENDIAN)
			value = __builtin_bswap16(value);

		(void)XXH32_update(&state, &value, sizeof(value));
	}
	return XXH32_digest(&state);
}


uint32_t cmp_hdr_checksum(uint32_t *checksum, const void *src, uint32_t src_size,
			  enum cmp_type src_type)
{
	struct sample_desc src_desc;
	uint32_t ret;

	if (!checksum)
		return CMP_ERROR(GENERIC);

	ret = sample_read_src_init(&src_desc, src, src_size, src_type);
	if (cmp_is_error_int(ret))
		return ret;

	*checksum = cmp_hdr_checksum_int(&src_desc);
	return CMP_ERROR(NO_ERROR);
}
