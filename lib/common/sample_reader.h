#ifndef SAMPLE_READER_H
#define SAMPLE_READER_H

#include <stdint.h>
#include <stddef.h>

#include "../cmp.h"
#include "err_private.h"
#include "compiler.h"
#include "bithacks.h"


struct sample_desc {
	const void *data;
	uint32_t num_samples;
	enum cmp_type dtype;
};


static __inline uint32_t cal_num_samples_round_up(uint32_t src_size, enum cmp_type src_type)
{
	switch (src_type) {
	case CMP_I16:
	case CMP_U16:
		if (src_size >= UINT32_MAX)
			return CMP_ERROR(HDR_ORIGINAL_TOO_LARGE);
		return div_round_up_u32(src_size, sizeof(int16_t));

	case CMP_I16_IN_I32:
		return div_round_up_u32(src_size, sizeof(int32_t));

	case CMP_RAW12:
		/* This is overflow safe version of (src_size * 2 / 3). */
		return (src_size / 3 * 2) + (src_size % 3 != 0);

	default:
		return CMP_ERROR(PARAMS_INVALID);
	}
}


static __inline uint32_t sample_read_src_init(struct sample_desc *src_desc, const void *src,
					      uint32_t src_size, enum cmp_type src_type)
{
	if (!src)
		return CMP_ERROR(SRC_NULL);

	if (src_size == 0)
		return CMP_ERROR(SRC_SIZE_WRONG);

	switch (src_type) {
	case CMP_I16:
	case CMP_U16:
		if (!is_multiple_of(src_size, sizeof(int16_t)))
			return CMP_ERROR(SRC_SIZE_WRONG);
		break;
	case CMP_I16_IN_I32:
		if (!is_multiple_of(src_size, sizeof(int32_t)))
			return CMP_ERROR(SRC_SIZE_WRONG);
		break;
	case CMP_RAW12:
		if (src_size % 3 == 1)
			return CMP_ERROR(SRC_SIZE_WRONG);
		break;
	default:
		return CMP_ERROR(PARAMS_INVALID);
	}

	src_desc->data = src;
	src_desc->num_samples = cal_num_samples_round_up(src_size, src_type);
	src_desc->dtype = src_type;

	return CMP_ERROR(NO_ERROR);
}


/**
 * @brief Reads a 16-bit signed integer from the sample data
 *
 * @param desc	pointer to the sample descriptor
 * @param i	index of the sample to read
 *
 * @return the 16-bit signed integer at index i
 */

static __inline int16_t sample_read_i16(const struct sample_desc *desc, uint32_t i)
{
	switch (desc->dtype) {
	case CMP_I16:
	case CMP_U16:
		return ((const int16_t *)desc->data)[i];
	case CMP_I16_IN_I32:
		return (int16_t)(((const int32_t *)desc->data)[i] & 0xFFFF);
	default: /*
		  * Setting the default here is slightly fast, however it is
		  * unreachable anyway, because sample_read_src_init() rejects
		  * any other dtype.
		  */
	case CMP_RAW12: {
		/*
		 * A sample is contained in exactly two bytes of the 3-byte
		 * group: an even sample in bytes 0 and 1, an odd sample in
		 * bytes 1 and 2. The byte offset of the first of the two bytes
		 * is (i / 2) * 3 + (i & 1), which is identical to i + (i / 2).
		 */
		const uint8_t *pair = (const uint8_t *)desc->data + i + (i >> 1);
		uint32_t bit_offset = (i & 1) << 2;

		return (int16_t)((((uint32_t)pair[0] >> bit_offset) |
				  ((uint32_t)pair[1] << (8 - bit_offset))) &
				 0x0FFFU);
	}
	}
}


static __inline uint32_t get_eff_bit_depth(const struct sample_desc *desc)
{
	switch (desc->dtype) {
	case CMP_I16:
	case CMP_U16:
	case CMP_I16_IN_I32:
		return bitsizeof(int16_t);
	case CMP_RAW12:
		return 12;
	default:
		return 0;
	}
}


static __inline uint32_t cal_packed_size(uint32_t num_samples, enum cmp_type dtype)
{
	switch (dtype) {
	case CMP_I16:
	case CMP_U16:
	case CMP_I16_IN_I32:
		return num_samples * sizeof(int16_t);
	case CMP_RAW12:
		return (num_samples / 2 * 3) + ((num_samples & 1) * 2);
	default:
		return 0;
	}
}


static __inline uint32_t get_packed_size(const struct sample_desc *desc)
{
	return cal_packed_size(desc->num_samples, desc->dtype);
}

#endif /* SAMPLE_READER_H */
