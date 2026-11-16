/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 *
 * @copyright GPL-2.0
 *
 * @brief Internal compression header functions
 */

#ifndef CMP_HEADER_PRIVATE_H
#define CMP_HEADER_PRIVATE_H

#include <stdint.h>

#include "../cmp.h"
#include "sample_reader.h"
#include "bitstream_writer.h"
#include "compiler.h"


/** Seed value used for initialising the checksum computation, arbitrarily chosen */
enum { CHECKSUM_SEED = 419764627 };


compile_time_assert(CMP_HDR_SIZE == 24, cmp_header_size_must_be_24_bytes);

compile_time_assert((CMP_HDR_BITS_PREPROCESSING + CMP_HDR_BITS_ENCODER_TYPE +
		     CMP_HDR_BITS_ORIGINAL_DTYPE) == 8,
		    ped_fields_must_fill_one_byte);


/**
 * @brief Serialize compression header to a byte buffer
 *
 * @param bs	pointer to an initialised bitstream writer structure
 * @param hdr	pointer to header structure to serialize
 *
 * @returns the compression header size or an error, which can be checked using
 *	cmp_is_error()
 */

uint32_t cmp_hdr_serialize(struct bitstream_writer *bs, const struct cmp_hdr *hdr);


/**
 * @brief Calculates data checksum
 *
 * @param desc	pointer to the sample descriptor
 *
 * @returns a 32-bit checksum of the data buffer
 */

uint32_t cmp_hdr_checksum_int(const struct sample_desc *desc);

#endif /* CMP_HEADER_PRIVATE_H */
