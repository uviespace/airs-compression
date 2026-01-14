/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 *
 * @copyright GPL-2.0
 *
 * @brief Compression header definitions
 *
 * Based on ARIEL-UVIE-PL-TN-004 Issue 0.2
 */

#ifndef CMP_HEADER_H
#define CMP_HEADER_H

#include <stdint.h>

#include "common/compiler.h"


/*
 * Bit length of the different header fields
 */
#define CMP_HDR_BITS_VERSION          16
#define CMP_HDR_BITS_COMPRESSED_SIZE  24
#define CMP_HDR_BITS_ORIGINAL_SIZE    24
#define CMP_HDR_BITS_CHECKSUM         32
#define CMP_HDR_BITS_IDENTIFIER       32
#define CMP_HDR_BITS_SEQUENCE_NUMBER  8
#define CMP_HDR_BITS_PREPROCESSING    3
#define CMP_HDR_BITS_ENCODER_TYPE     2
#define CMP_HDR_BITS_ORIGINAL_DTYPE   3
#define CMP_HDR_BITS_ENCODER_PARAM    16
#define CMP_HDR_BITS_ENCODER_OUTLIER  24
#define CMP_HDR_BITS_PREPROCESS_PARAM 8


/*
 * Byte offsets of the different header fields
 */
#define CMP_HDR_OFFSET_VERSION          0
#define CMP_HDR_OFFSET_COMPRESSED_SIZE  2
#define CMP_HDR_OFFSET_ORIGINAL_SIZE    5
#define CMP_HDR_OFFSET_CHECKSUM         8
#define CMP_HDR_OFFSET_IDENTIFIER       12
#define CMP_HDR_OFFSET_SEQUENCE_NUMBER  16
#define CMP_HDR_OFFSET_PED_FIELDS       17 /* combined: preprocessing, encoder type, original dtype  */
#define CMP_HDR_OFFSET_ENCODER_PARAM    18
#define CMP_HDR_OFFSET_OUTLIER_PARAM    20
#define CMP_HDR_OFFSET_PREPROCESS_PARAM 23


/*
 * Maximum values that can be stored in the size fields
 */
#define CMP_HDR_MAX_COMPRESSED_SIZE ((1ULL << CMP_HDR_BITS_COMPRESSED_SIZE) - 1)
#define CMP_HDR_MAX_ORIGINAL_SIZE   ((1ULL << CMP_HDR_BITS_ORIGINAL_SIZE) - 1)


/** Size of the compression header in bytes */
#define CMP_HDR_SIZE                                                                            \
	((CMP_HDR_BITS_VERSION + CMP_HDR_BITS_COMPRESSED_SIZE + CMP_HDR_BITS_ORIGINAL_SIZE +    \
	  CMP_HDR_BITS_CHECKSUM + CMP_HDR_BITS_IDENTIFIER + CMP_HDR_BITS_SEQUENCE_NUMBER +      \
	  CMP_HDR_BITS_PREPROCESSING + CMP_HDR_BITS_ENCODER_TYPE + CMP_HDR_BITS_ENCODER_PARAM + \
	  CMP_HDR_BITS_ENCODER_OUTLIER + CMP_HDR_BITS_ORIGINAL_DTYPE +                          \
	  CMP_HDR_BITS_PREPROCESS_PARAM) /                                                      \
	 8)
compile_time_assert(CMP_HDR_SIZE == 24, cmp_header_size_must_be_24_bytes);

#endif /* CMP_HEADER_H */
