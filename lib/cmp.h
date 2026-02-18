/**
 * @mainpage  AIRS PortAble Compression Engine (AIRSPACE)
 * Please see @ref cmp.h for the compression API documentation.
 *
 * @file cmp.h
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Compression API
 *
 * - Setup: Create a compression context with cmp_initialise()
 * - Process: Compress data using cmp_compress_u16()
 * - Reset compression context using cmp_reset()
 * - Clean-up: Optionally destroy context with cmp_deinitialise()
 *
 * @see @ref examples/ directory for usage examples
 *
 * @warning The interface is not frozen yet and may change in future versions.
 * Changes may include:
 *    - New compression parameters
 *    - Change/Extended API functionality
 */

#ifndef CMP_H
#define CMP_H

#include <stdint.h>

#include "cmp_header.h"

/* ====== Utility Macros  ====== */
/** Convert a token to a string literal */
#define CMP_QUOTE(str)            #str
/** Expand a macro and convert the result to a string literal */
#define CMP_EXPAND_AND_QUOTE(str) CMP_QUOTE(str)

/* ====== Version Information ====== */
#define CMP_VERSION_MAJOR   0 /**< major part of the version ID */
#define CMP_VERSION_MINOR   7 /**< minor part of the version ID */
#define CMP_VERSION_RELEASE 1 /**< release part of the version ID */

/**
 * @brief Complete version number
 */
#define CMP_VERSION_NUMBER \
	(CMP_VERSION_MAJOR * 100 * 100 + CMP_VERSION_MINOR * 100 + CMP_VERSION_RELEASE)

/**
 * @brief Complete version string
 */
#define CMP_VERSION_STRING \
	CMP_EXPAND_AND_QUOTE(CMP_VERSION_MAJOR.CMP_VERSION_MINOR.CMP_VERSION_RELEASE)

/* ====== Parameter Selection ====== */
/**
 * @brief Preprocessing techniques for encoding
 *
 * This enum defines the available preprocessing methods that can be applied
 * before encoding. Preprocessing techniques aim to improve compression
 * efficiency.
 */

enum cmp_preprocessing {
	CMP_PREPROCESS_NONE, /**< No preprocessing is applied to the data */
	CMP_PREPROCESS_DIFF, /**< Differences between neighbouring values are computed */
	CMP_PREPROCESS_IWT,  /**< Integer Wavelet Transform preprocessing */
	CMP_PREPROCESS_MODEL /**< Subtracts a model based on previously compressed data,
			      *   only allowed as a secondary preprocessing step
			      */
};


/**
 * @brief Available compression encoders
 */

enum cmp_encoder_type {
	CMP_ENCODER_UNCOMPRESSED, /**< Uncompressed mode */
	CMP_ENCODER_GOLOMB_ZERO,  /**< Golomb encoder with zero escape mechanism */
	CMP_ENCODER_GOLOMB_MULTI  /**< Golomb encoder with multi escape mechanism */
};


/**
 * @brief Compression parameters
 *
 * Supports independent configuration of preprocessing and encoders
 * for a primary compression pass and optional secondary passes.
 *
 * @warning Parameter names and behaviour may change in future versions.
 */

struct cmp_params {
	/*
	 * Primary (initial pass) settings
	 */
	enum cmp_preprocessing primary_preprocessing; /**< Preprocessing for the first pass */
	enum cmp_encoder_type primary_encoder_type;   /**< Encoder used in the first pass */
	uint32_t primary_encoder_param;               /**< Parameter for the primary encoder */
	uint32_t primary_encoder_outlier; /**< Primary outlier parameter for CMP_ENCODER_GOLOMB_MULTI */

	/*
	 * Secondary (subsequent passes) settings (if any)
	 */
	uint32_t secondary_iterations;                  /**< Max secondary passes (0 = disabled) */
	enum cmp_preprocessing secondary_preprocessing; /**< Preprocessing for secondary passes */
	enum cmp_encoder_type secondary_encoder_type;   /**< Encoder for secondary passes */
	uint32_t secondary_encoder_param;               /**< Parameter for the secondary encoder */
	uint32_t secondary_encoder_outlier; /**< Secondary outlier parameter for CMP_ENCODER_GOLOMB_MULTI */
	uint32_t model_rate; /**< Model adaptation rate (used with CMP_PREPROCESS_MODEL) */

	/* Additional Options */
	uint8_t checksum_enabled; /**< Enable checksum generation of original data if non-zero */
	uint8_t uncompressed_fallback_enabled; /**< Fall back to uncompressed storage if compression is ineffective */
};


/**
 * @brief Compression context
 *
 * This structure maintains the state of an ongoing compression process.
 *
 * @warning This structure MUST NOT be directly manipulated by external code.
 *	Always use the provided API functions to interact with the compression
 *	context.
 */

struct cmp_context {
	uint32_t magic;           /**< Magic number to prevent use of uninitialised contexts */
	struct cmp_params params; /**< Compression parameters used in the current context */
	void *work_buf;           /**< Pointer to the working buffer */
	uint32_t work_buf_size;   /**< Size of the working buffer in bytes */
	uint32_t model_size;      /**< Size of the model used in the model-based preprocessing */
	uint32_t identifier;      /**< Identifier for the compression model */
	uint8_t sequence_number; /**< Number of compression passes performed since the last reset */
};


/* ====== Compression Helper Functions ====== */
/**
 * @brief Tells if a result is an error code
 *
 * @param code	return value to check
 *
 * @returns non-zero if the code is an error
 */

unsigned int cmp_is_error(uint32_t code);


/**
 * @brief Get the maximum compressed size in a worst-case scenario
 *
 * In this scenario the input data are not compressible. This function is
 * primarily useful for memory allocation purposes (destination buffer size).
 * Assumes a worst case configuration.
 *
 * @param packed_size	packed size of the data in bytes (same as src_size,
 *			except for cmp_compress_i16_in_i32() where it's half)
 *
 * @returns the compressed size in the worst-case scenario or an error if the
 *	bound size is larger than the maximum compressed size
 *	(CMP_HDR_MAX_COMPRESSED_SIZE), which can be checked using cmp_is_error()
 */

uint32_t cmp_compress_bound(uint32_t packed_size);


/**
 * @brief Calculate the maximum buffer size required for uncompressed storage
 *
 * This macro is useful for (static) allocation of the compression destination
 * buffer when using uncompressed storage.
 *
 * It helps prevent compression failures due to insufficient destination buffer
 * space in the following scenarios:
 * - When explicitly using uncompressed mode (CMP_ENCODER_UNCOMPRESSED)
 * - When uncompressed_fallback_enabled is set
 *
 * In all other compression scenarios, use cmp_compress_bound(), which provides
 * an upper bound assuming the worst-case compression ratio.
 *
 * @param packed_size	packed size of the data in bytes (same as src_size,
 *			except for cmp_compress_i16_in_i32() where it's half)
 *
 * @returns the buffer size needed for uncompressed storage, or SIZE_MAX if
 *	the source size is too large
 */

#define CMP_UNCOMPRESSED_BOUND(packed_size)                            \
	((packed_size) <= CMP_HDR_MAX_COMPRESSED_SIZE - CMP_HDR_SIZE ? \
		 CMP_HDR_SIZE + (packed_size) :                        \
		 SIZE_MAX)


/**
 * @brief Calculates the size needed for the compression working buffer
 *
 * Some preprocessing methods (like CMP_PREPROCESS_MODEL) need extra memory to
 * store intermediate calculations or a predictive model. The working buffer
 * provides that temporary storage space.
 *
 * @param params	pointer to a compression parameters struct used to
 *			compress the data
 * @param src_size	size of a source data buffer in bytes
 *
 * @returns the minimum size in bytes needed for a compression working buffer
 *	(can be 0 if no working buffer is needed) or an error, which can be
 *	checked using cmp_is_error()
 */

uint32_t cmp_cal_work_buf_size(const struct cmp_params *params, uint32_t src_size);


/* ======   Compression Functions   ====== */
/**
 * @brief Initialises a compression context
 *
 * The compression context stores the configuration and internal state required
 * for a compression. It must be initialised before any data is compressed. The
 * context MUST NOT be read or manipulated by any external code. Always use the
 * provided API functions to interact with it.
 *
 * @param ctx		pointer to a compression context struct to initialise
 * @param params	pointer to a compression parameters struct used to
 *			compress the data
 * @param work_buf	pointer to a working buffer (can be NULL if
 *			work_buf_size is 0)
 * @param work_buf_size	size of the working buffer in bytes; needed size can be
 *			calculated with cmp_cal_work_buf_size(params, src_size)
 *
 * @warning The caller is responsible for managing the memory of the working
 *	buffer. It must remain valid for the entire lifetime of the context, as
 *	the library only stores a pointer to it.
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

uint32_t cmp_initialise(struct cmp_context *ctx, const struct cmp_params *params, void *work_buf,
			uint32_t work_buf_size);

/**
 * @brief Compresses a signed 16-bit data buffer
 *
 * You can use this function repeatedly to compress more data of the same size.
 *
 * @param ctx		pointer to a compression context; must have been
 *			initialised once with cmp_initialise()
 * @param dst		the buffer to compress the src buffer into, MUST be
 *			8-byte aligned
 * @param dst_capacity	size of the dst buffer; may be any size, but
 *			cmp_compress_bound(src_size) is guaranteed to be large
 *			enough
 * @param src		pointer to the data to compress
 * @param src_size	size of the data to compress, must be the same for every
 *			source buffer until the context is reset
 *
 * @returns the compressed size or an error, which can be checked using
 *	cmp_is_error()
 */

uint32_t cmp_compress_i16(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
			  const int16_t *src, uint32_t src_size);


/**
 * @brief Compresses 16-bit signed data packed in 32-bit words
 *
 * Same as cmp_compress_i16() but for int16_t data packed into int32_t words.
 * Useful when data is stored in a 32-bit buffer but only the lower 16 bits are
 * used.
 *
 * @note Only the lower 16 bits of each 32-bit word are used; the upper 16 bits
 *	are ignored.
 */

uint32_t cmp_compress_i16_in_i32(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
				 const int32_t *src, uint32_t src_size);


/**
 * @brief Compresses an unsigned 16-bit data buffer
 *
 * Same as cmp_compress_i16() but for uint16_t data.
 */

uint32_t cmp_compress_u16(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
			  const uint16_t *src, uint32_t src_size);


/**
 * @brief Resets the compression context
 *
 * This ensures that the next compression uses the primary parameters.
 * If used with MODEL preprocessing, it clears the internal predictive model,
 * allowing you to start fresh with new data. Use this when compressing
 * independent datasets or when you want to ensure that the model does not
 * carry over patterns from previous data, for example at the start of a new
 * acquisition series.
 *
 * @param ctx	pointer to a compression context to reset
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

uint32_t cmp_reset(struct cmp_context *ctx);


/**
 * @brief Destroys a compression context
 *
 * Ends the lifetime of the compression context and discards all internal
 * state. After this call, the context is no longer initialised and must be
 * reinitialised with cmp_initialise() before further use.
 *
 * @note This function does NOT free any caller-owned memory such as the working
 *	buffer or any other buffers.
 *
 * @param ctx	pointer to the compression context to be destroyed
 */

void cmp_deinitialise(struct cmp_context *ctx);


/* ======  Compression Header Functions   ====== */
/**
 * @brief Data type of the original uncompressed data
 */

enum cmp_type {
	CMP_I16,        /**< Signed 16-bit integers */
	CMP_I16_IN_I32, /**< Signed 16-bit integers packed in 32-bit words */
	CMP_U16         /**< Unsigned 16-bit integers */
};


/**
 * @brief Compression header fields
 *
 * @note This is not the on-disk format - use cmp_hdr_serialize() and
 * cmp_hdr_deserialize() for conversion to/from the binary format.
 */

struct cmp_hdr {
	uint16_t version;
	uint32_t compressed_size;
	uint32_t original_size;
	uint32_t checksum;
	uint32_t identifier;
	uint8_t sequence_number;
	enum cmp_preprocessing preprocessing;
	enum cmp_encoder_type encoder_type;
	uint32_t encoder_param;
	uint32_t encoder_outlier;
	enum cmp_type original_dtype;
	uint32_t preprocess_param;
};


/**
 * @brief Deserialize compression header
 *
 * @param buf		buffer containing the serialized header (may be a
 *			compressed data buffer)
 * @param buf_size	size of buffer
 * @param hdr		pointer to header structure to fill
 *
 * @note Only the version field is valid for headers with version 0.6 and earlier.
 *
 * @returns the compression header size or an error, which can be checked using
 *	cmp_is_error()
 */

uint32_t cmp_hdr_deserialize(const void *buf, uint32_t buf_size, struct cmp_hdr *hdr);


/**
 * @brief Computes the checksum in the header from the original uncompressed data
 *
 * @param checksum	pointer to the variable receiving the computed checksum
 * @param src		pointer to the original uncompressed data buffer
 * @param src_size	size of the data buffer in bytes
 * @param src_type	type of the data; CMP_I16 and CMP_U16 are treated
 *			identically for checksum purposes
 *
 * @returns an error code which can be checked using cmp_is_error()
 */

uint32_t cmp_hdr_checksum(uint32_t *checksum, const void *src, uint32_t src_size,
			  enum cmp_type src_type);


/**
 * @brief Set the sequence identifier counter
 *
 * The identifier is part of the compression header to mark a compression
 * sequence: a series of compressions that share the same model/state.
 * It remains constant within a sequence (while the sequence number increments
 * with each compression).
 *
 * Default start value is 0. When a program restarts, the identifier counter
 * starts at 0 again, so identifiers can repeat across independent program
 * executions. Use this function to choose a non-zero start value when you
 * need to avoid collisions across independent executions.
 *
 * @param identifier	value to set the identifier counter
 */

void cmp_hdr_set_identifier(uint32_t identifier);

#endif /* CMP_H */
