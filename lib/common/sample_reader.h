#ifndef SAMPLE_READER_H
#define SAMPLE_READER_H

#include <stdint.h>

#include "../cmp.h"
#include "err_private.h"


struct sample_desc {
	const void *data;
	uint32_t num_samples;
	uint8_t stride;
	enum cmp_type type;
};


static __inline uint32_t sample_read_src_init(struct sample_desc *src_desc, const void *src,
					      uint32_t src_size, enum cmp_type src_type)
{
	uint8_t stride;

	if (!src)
		return CMP_ERROR(SRC_NULL);

	if (src_size == 0)
		return CMP_ERROR(SRC_SIZE_WRONG);

	switch (src_type) {
	case CMP_I16:
	case CMP_U16:
		stride = sizeof(int16_t);
		break;
	case CMP_I16_IN_I32:
		stride = sizeof(int32_t);
		break;
	default:
		return CMP_ERROR(SRC_SIZE_WRONG);
	};

	if (src_size % stride != 0)
		return CMP_ERROR(SRC_SIZE_WRONG);

	src_desc->data = src;
	src_desc->num_samples = src_size / stride;
	src_desc->stride = stride;
	src_desc->type = src_type;

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
	const void *addr = (const uint8_t *)desc->data + (i * desc->stride);

	/* Assume samples size == stride size */
	if (desc->stride == sizeof(int32_t))
		return (int16_t)(*(const uint32_t *)addr & 0xFFFFU);

	return *(const int16_t *)addr;
}


static __inline uint32_t get_packed_size(const struct sample_desc *desc)
{
	return desc->num_samples * sizeof(int16_t);
}

#endif /* SAMPLE_READER_H */
