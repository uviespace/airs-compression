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


/** Seed value used for initializing the checksum computation, arbitrarily chosen*/
#define CHECKSUM_SEED 419764627


compile_time_assert(CMP_HDR_SIZE == 24, cmp_header_size_must_be_24_bytes);


/**
 * @brief serialize compression header to a byte buffer
 *
 * @param bs	Pointer to a initialized bitstream writer structure
 * @param hdr	Pointer to header structure to serialize
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
