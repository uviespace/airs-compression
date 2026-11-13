/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Binary compression header format definitions
 * @see @ref format for the complete header layout and field descriptions.
 *
 * Most users do not need to include this header directly.
 * @warning These definitions are part of the serialized format contract. Changes
 *	to field sizes or offsets may affect compatibility with stored data.
 */

#ifndef CMP_HEADER_H
#define CMP_HEADER_H


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
/* PED: preprocessing, encoder type, original dtype  */
#define CMP_HDR_OFFSET_PED_FIELDS       17
#define CMP_HDR_OFFSET_ENCODER_PARAM    18
#define CMP_HDR_OFFSET_OUTLIER_PARAM    20
#define CMP_HDR_OFFSET_PREPROCESS_PARAM 23


/*
 * Maximum values that can be stored in the size fields
 */
#define CMP_HDR_MAX_COMPRESSED_SIZE ((1UL << CMP_HDR_BITS_COMPRESSED_SIZE) - 1)
#define CMP_HDR_MAX_ORIGINAL_SIZE   ((1UL << CMP_HDR_BITS_ORIGINAL_SIZE) - 1)


/** Size of the compression header in bytes */
#define CMP_HDR_SIZE                                                                            \
	((CMP_HDR_BITS_VERSION + CMP_HDR_BITS_COMPRESSED_SIZE + CMP_HDR_BITS_ORIGINAL_SIZE +    \
	  CMP_HDR_BITS_CHECKSUM + CMP_HDR_BITS_IDENTIFIER + CMP_HDR_BITS_SEQUENCE_NUMBER +      \
	  CMP_HDR_BITS_PREPROCESSING + CMP_HDR_BITS_ENCODER_TYPE + CMP_HDR_BITS_ENCODER_PARAM + \
	  CMP_HDR_BITS_ENCODER_OUTLIER + CMP_HDR_BITS_ORIGINAL_DTYPE +                          \
	  CMP_HDR_BITS_PREPROCESS_PARAM) /                                                      \
	 8)


/* Minimum supported header version  */
#define CMP_MIN_SUPPORTED_VERSION_MAJOR   0
#define CMP_MIN_SUPPORTED_VERSION_MINOR   7
#define CMP_MIN_SUPPORTED_VERSION_RELEASE 0
#define CMP_MIN_SUPPORTED_VERSION                                                              \
	((CMP_MIN_SUPPORTED_VERSION_MAJOR * 10000) + (CMP_MIN_SUPPORTED_VERSION_MINOR * 100) + \
	 CMP_MIN_SUPPORTED_VERSION_RELEASE)

#endif /* CMP_HEADER_H */
