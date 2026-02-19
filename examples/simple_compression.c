/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Simple data compression example
 *
 * This example demonstrates how to use the compression library step by step.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <cmp.h>
#include <cmp_errors.h>
#include <cmp_header.h>

static void print_hex_dump(const uint8_t *data, uint32_t size);


/**
 * @brief demonstrate compression API usage
 */

static int simple_compression(void)
{
	/* Define constants for our example */
	enum {
		DATA_SAMPLES_EXAMPLE = 3, /* We compress 3 uint16_t values */
		DATA_SRC_SIZE_EXAMPLE = DATA_SAMPLES_EXAMPLE * sizeof(uint16_t)
	};

	struct cmp_params params = { 0 }; /* Parameters for compression configuration */
	struct cmp_context ctx = { 0 };   /* Private context to maintain compression state */

	uint8_t *dst = NULL;   /* Destination buffer to store the compressed data */
	uint32_t dst_capacity; /* Maximum size of the destination buffer */

	void *work_buf = NULL;  /* Internal working buffer needed by the compressor */
	uint32_t work_buf_size; /* Size of the internal working buffer */

	uint32_t cmp_size; /* Actual size of compressed data */
	uint32_t return_value;


	/*
	 * Step 0: Set the sequence identifier starting value (optional)
	 * The identifier in the compression header marks a compression sequence:
	 * a series of compressions that share the same model/state. It remains
	 * constant within a sequence while the sequence number increments with
	 * each compression.
	 *
	 * Default start value is 0. When a program restarts, the identifier
	 * counter starts at 0 again, so identifiers can repeat across independent
	 * executions. Set this only when you need a non-zero start value to avoid
	 * collisions across independent runs.
	 *
	 * This is only for demonstration; setting the identifier to a fixed
	 * value is as effective as letting it start at 0.
	 */
	cmp_hdr_set_identifier(0xDEADCAFE);


	/*
	 * Step 1: Configure Compression Parameters
	 * In the example, we use a configuration with a predictive model.
	 * This configuration is ideal for compressing time-series data where
	 * values change gradually over time.
	 * On the first compression, no predictive model of the data is available,
	 * therefore a multi-pass approach is used:
	 * - First pass: Uses primary compression parameters without a model
	 * - Subsequent passes: Use secondary compression parameters with a model
	 * If a model compression is not desired, the secondary parameters can be
	 * disabled by setting secondary_iterations = 0
	 *
	 * The parameter values shown are example values. You may need to tune
	 * these based on your specific data characteristics.
	 */
	memset(&params, 0, sizeof(params));

	/* Primary compression parameters - used for the first compression pass */
	/* DIFF preprocessing: Calculates differences between neighbouring values */
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	/* GOLOMB_ZERO encoder: Golomb encoder with zero escape mechanism */
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 1055; /* Golomb encoder parameter */
	params.primary_encoder_outlier = 0;  /* Not used by GOLOMB_ZERO encoder */

	/* Secondary compression parameters - used for subsequent compression passes */
	params.secondary_iterations = 15;
	/* Secondary_iterations: Controls the lifetime of the predictive model.
	 * It sets the maximum number of times that secondary parameters can be
	 * used in a row. Once this limit has been reached, the context
	 * automatically resets, forcing the next compression to use the primary
	 * parameters. Setting it to 0 disables the secondary compression stage
	 * entirely.
	 */
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	/* MODEL preprocessing: Uses a predictive model based on previous compressed data */
	params.secondary_encoder_type = CMP_ENCODER_GOLOMB_MULTI;
	/* GOLOMB_MULTI encoder: Golomb encoder with multi escape mechanism */
	params.secondary_encoder_param = 8;     /* Golomb encoder parameter */
	params.secondary_encoder_outlier = 107; /* Multi escape mechanism parameter */
	params.model_rate = 11;                 /* Model adaptation rate */

	/* Fallback option: If compression doesn't save any space, store the
	 * data uncompressed, can be disabled if compression speed is critical
	 */
	params.uncompressed_fallback_enabled = 1;
	/* Add checksum: Verify that decompressed data matches original data */
	params.checksum_enabled = 1; /* Can be disabled if compression speed is critical */


	/*
	 * Step 2: Allocate Working Buffer
	 * Some preprocessing methods (like MODEL) need extra memory to store
	 * intermediate calculations or a predictive model. This working buffer
	 * provides that temporary storage space.
	 * The caller is responsible for managing the memory of the working
	 * buffer. It must remain valid for the entire lifetime of the context,
	 * as the library only stores a pointer to it.
	 */
	work_buf_size = cmp_cal_work_buf_size(&params, DATA_SRC_SIZE_EXAMPLE);
	/* NOTE: All return values of compression functions must be checked
	 * with cmp_is_error() to check if they were successful.
	 */
	if (cmp_is_error(work_buf_size)) {
		fprintf(stderr, "Error calculating working buffer size: %s. (Error Code: %u)\n",
			cmp_get_error_message(work_buf_size), cmp_get_error_code(work_buf_size));
		return -1;
	}

	/* Allocate working buffer if needed */
	if (work_buf_size > 0) {
		work_buf = malloc(work_buf_size);
		if (work_buf == NULL) {
			fprintf(stderr, "Memory allocation failed for working buffer\n");
			return -1;
		}
	}


	/*
	 * Step 3: Allocate Destination Buffer
	 * We need to allocate space for the compressed output. The library
	 * provides a function to calculate the maximum possible compressed
	 * size, ensuring we have enough space even in worst-case scenarios.
	 */
	dst_capacity = cmp_compress_bound(DATA_SRC_SIZE_EXAMPLE);
	if (cmp_is_error(dst_capacity)) {
		if (cmp_get_error_code(dst_capacity) == CMP_ERR_HDR_CMP_SIZE_TOO_LARGE) {
			/* Fallback: Use maximum allowed compressed size when source is too large */
			dst_capacity = CMP_HDR_MAX_COMPRESSED_SIZE;
			fprintf(stderr,
				"Warning: Source data size too large for cmp_compress_bound(). "
				"Use fallback destination buffer size of CMP_HDR_MAX_COMPRESSED_SIZE.\n"
				"Compressed data may not fit into the destination buffer!\n");
		} else {
			fprintf(stderr,
				"Error calculating destination buffer size: %s. (Error Code: %u)\n",
				cmp_get_error_message(dst_capacity),
				cmp_get_error_code(dst_capacity));
			free(work_buf);
			return -1;
		}
	}

	/*
	 * Allocate destination buffer for the compressed output
	 * NOTE: The destination buffer (dst) must be 8-byte aligned.
	 */

	dst = malloc(dst_capacity);
	if (dst == NULL) {
		fprintf(stderr, "Memory allocation failed for destination buffer\n");
		free(work_buf);
		return -1;
	}


	/*
	 * Step 4: Initialise Compression Context
	 * The compression context stores the configuration and internal state
	 * required for a compression. It must be initialised before any data is
	 * compressed. The context MUST NOT be read or manipulated by any
	 * external code. Always use the provided API functions to interact with
	 * it.
	 */
	return_value = cmp_initialise(&ctx, &params, work_buf, work_buf_size);
	if (cmp_is_error(return_value)) {
		/*
		 * Debug Helpers:
		 * cmp_get_error_message(code): Returns a human-readable, read-
		 * only string describing the error. Do not modify or free it.
		 * cmp_get_error_code(code): Returns a stable numeric error ID
		 * (see cmp_errors.h) for programmatic handling.
		 */
		fprintf(stderr, "Compression initialisation failed: %s. (Error Code: %u)\n",
			cmp_get_error_message(return_value), cmp_get_error_code(return_value));
		free(dst);
		free(work_buf);
		return -1;
	}


	/*
	 * Step 5: Compress Data
	 * Compress the sample data using the configured compression parameters.
	 * The first call will use the primary compression settings.
	 */
	{
		/*
		 * Sample data to compress - adjust this to your actual data.
		 * This example uses a simple array.
		 */
		uint16_t sample_data[DATA_SAMPLES_EXAMPLE] = { 0x0000, 0x0001, 0x0002 };
		uint32_t sample_data_size = sizeof(sample_data);

		/* First compression call uses primary parameters (DIFF + GOLOMB_ZERO) */
		cmp_size = cmp_compress_u16(&ctx, dst, dst_capacity, sample_data, sample_data_size);
		if (cmp_is_error(cmp_size)) { /* check compression result */
			fprintf(stderr, "Data compression failed: %s. (Error Code: %u)\n",
				cmp_get_error_message(cmp_size), cmp_get_error_code(cmp_size));
			free(dst);
			free(work_buf);
			return -1;
		}
	}


	/*
	 * Step 6: Use the Compression Results
	 * In this example, we simply print the compressed data.
	 * In real applications, you would save this or transmit it.
	 */
	{
		printf("1st Compressed Data (Size: %" PRIu32 " bytes):\n", cmp_size);
		print_hex_dump(dst, cmp_size);
	}


	/*
	 * Repeat Steps 5 and 6 until you have compressed enough data.
	 * For demonstration, we'll compress another chunk to show how
	 * secondary compression parameters are applied.
	 */
	{
		uint16_t sample_data2[DATA_SAMPLES_EXAMPLE] = { 0x0002, 0x0001, 0x0001 };
		uint32_t sample_data2_size = sizeof(sample_data2);

		/* This call will use secondary parameters (MODEL + GOLOMB_MULTI) */
		cmp_size =
			cmp_compress_u16(&ctx, dst, dst_capacity, sample_data2, sample_data2_size);
		if (cmp_is_error(cmp_size)) { /* check compression result */
			fprintf(stderr, "Data compression failed: %s. (Error Code: %u)\n",
				cmp_get_error_message(cmp_size), cmp_get_error_code(cmp_size));
			free(dst);
			free(work_buf);
			return -1;
		}

		printf("2nd Compressed Data (Size: %" PRIu32 " bytes):\n", cmp_size);
		print_hex_dump(dst, cmp_size);
	}


	/*
	 * Step 7: Reset Compression Context
	 * Reset clears the internal predictive model, allowing you to start
	 * fresh with new data. Use this when compressing independent data sets
	 * or when you want to ensure the model doesn't carry over patterns
	 * from previous data, e.g. at the start of a new acquisition series.
	 */

	return_value = cmp_reset(&ctx);
	if (cmp_is_error(return_value)) {
		fprintf(stderr, "Context reset failed: %s. (Error Code: %u)\n",
			cmp_get_error_message(return_value), cmp_get_error_code(return_value));
		free(dst);
		free(work_buf);
		return -1;
	}


	/*
	 * Step 8: Compress Additional Data (not shown)
	 * After a reset, the next compression call will use primary parameters
	 * again, as if starting fresh (similar to Step 5).
	 */


	/*
	 * Step 9: Clean-up
	 * Free all allocated memory and deinitialise the compression context.
	 */
	cmp_deinitialise(&ctx); /* Optional: destroys the state of the context */
	free(dst);
	free(work_buf);

	return 0;
}


/**
 * @brief print hex dump of binary data
 *
 * @param data pointer to binary data
 * @param size number of bytes to display
 */

static void print_hex_dump(const uint8_t *data, uint32_t size)
{
	uint32_t i;

	for (i = 0; i < size; i++)
		printf("%02X%s", data[i], ((i + 1) % 32 == 0) ? "\n" : " ");

	if (size % 32 != 0)
		printf("\n");
}


/**
 * @brief main function of the compression example
 *
 * @returns EXIT_SUCCESS if the example succeeds, EXIT_FAILURE otherwise
 */

int main(void)
{
	if (simple_compression())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
