/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief AIRSPACE CLI - A tool for (de)compressing AIRS science data
 */

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/cmp.h"
#include "file.h"
#include "log.h"
#include "util.h"
#include "params_parse.h"

/* Program information */
#define PROGRAM_NAME "AIRSPACE CLI"
#ifndef AIRSPACE_VERSION
#  define AIRSPACE_VERSION "v" CMP_VERSION_STRING
#endif
#define AIRSPACE_EXTENSION ".air"

#define AUTHOR "Dominik Loidolt"
#define AIRSPACE_WELCOME_MESSAGE                                                    \
	"*** %s (%d-bit) %s, by %s ***\n", PROGRAM_NAME, (int)(sizeof(size_t) * 8), \
		AIRSPACE_VERSION, AUTHOR

/** Operation modes */
enum operation_mode { MODE_COMPRESS, MODE_DECOMPRESS };


/* memory allocation or die */
static void *malloc_safe(size_t size)
{
	void *ptr = malloc(size);

	if (!ptr) {
		LOG_ERROR_WITH_ERRNO("Memory allocation failed for size %lu", (unsigned long)size);
		exit(EXIT_FAILURE);
	}
	return ptr;
}


/**
 * @brief appends the airspace specific suffix to the input string
 *
 * Adds a predefined suffix to the given string, using an internal buffer.
 *
 * @param str	string to add suffix or NULL to free resources
 *
 * @returns pointer to the modified string
 * @warning The buffer is shared across calls and not thread-safe.
 */

static const char *add_airspace_suffix(const char *str)
{
	static char *buf;
	static size_t buf_size;

	size_t str_len;
	size_t need_buf_size;

	if (!str) {
		free(buf);
		return NULL;
	}

	str_len = strlen(str);
	need_buf_size = str_len + sizeof(AIRSPACE_EXTENSION);

	if (need_buf_size > buf_size) {
		enum { BUFFER_MARGIN = 30 };

		free(buf);
		buf_size = need_buf_size + BUFFER_MARGIN;
		buf = malloc_safe(buf_size);
	}

	memcpy(buf, str, str_len);
	memcpy(buf + str_len, AIRSPACE_EXTENSION, sizeof(AIRSPACE_EXTENSION));

	return buf;
}


static void log_file_status(enum log_level level, const char *input_filename, uint32_t input_size,
			    const char *output_name, uint32_t output_size)
{
	int const verbose = log_get_level() > LOG_LEVEL_DEBUG;
	struct hr_fmt const hr_i = util_make_human_readable(input_size, verbose);
	struct hr_fmt const hr_o = util_make_human_readable(output_size, verbose);

	LOG_PLAIN(level, "%s: %.2f%% (%.*f%s => %.*f%s, %s)\n", input_filename,
		  (double)output_size / (double)input_size * 100.0,
		  hr_i.precision, hr_i.value, hr_i.suffix,
		  hr_o.precision, hr_o.value, hr_o.suffix, output_name);
}


static void log_summery(const char **input_files, int num_files, size_t sum_input_size,
			const char *output_name, size_t sum_output_size)
{
	if (num_files == 1) { /* one file -> display the file status instead of the summery */
		/* if not already done in the log file status */
		if (log_get_level() < LOG_LEVEL_DEBUG) {
			log_file_status(LOG_LEVEL_INFO, input_files[0], (uint32_t)sum_input_size,
					output_name, (uint32_t)sum_output_size);
		}
	} else {
		int const verbose = log_get_level() > LOG_LEVEL_DEBUG;
		struct hr_fmt const hr_i_sum = util_make_human_readable(sum_input_size, verbose);
		struct hr_fmt const hr_o_sum = util_make_human_readable(sum_output_size, verbose);

		LOG_PLAIN(LOG_LEVEL_INFO, "%d files compressed: %.2f%% (%.*f%s => %.*f%s)\n",
			  num_files, (double)sum_output_size / (double)sum_input_size * 100.0,
			  hr_i_sum.precision, hr_i_sum.value, hr_i_sum.suffix,
			  hr_o_sum.precision, hr_o_sum.value, hr_o_sum.suffix);
	}
}


static int compress_file_list(const char *output_name, const char **input_files, int num_files,
			      const struct cmp_params *params)
{
	int result = EXIT_FAILURE;
	int const needs_output_name = !output_name;
	int i;

	void *work_buf = NULL;
	struct cmp_context ctx;

	size_t sum_input_size = 0;
	size_t sum_output_size = 0;

	assert(input_files);
	assert(num_files > 0);
	assert(params);

	{ /* Initialization setup */
		uint32_t first_file_size;
		uint32_t work_buf_size;
		uint32_t return_code;

		/* Allocate work buff if need */
		if (file_get_size_u32(input_files[0], &first_file_size))
			goto cleanup;
		work_buf_size = cmp_cal_work_buf_size(params, first_file_size);
		if (cmp_is_error(work_buf_size)) {
			LOG_ERROR_CMP(work_buf_size, "Error calculating work buffer size");
			goto cleanup;
		}
		if (work_buf_size > 0)
			work_buf = malloc_safe(work_buf_size);

		return_code = cmp_initialise(&ctx, params, work_buf, work_buf_size);
		if (cmp_is_error(return_code)) {
			LOG_ERROR_CMP(return_code, "Compression initialization failed");
			goto cleanup;
		}
	}

	for (i = 0; i < num_files; i++) {
		uint32_t output_size;

		assert(input_files[i]);
		if (needs_output_name)
			output_name = add_airspace_suffix(input_files[i]);

		output_size = file_compress(&ctx, output_name, input_files[i]);
		if (cmp_is_error(output_size))
			goto cleanup;

		{ /* compression done; do some longing */
			uint32_t input_size;

			(void)file_get_size_u32(input_files[i], &input_size);
			log_file_status(LOG_LEVEL_DEBUG, input_files[i], input_size, output_name,
					output_size);
			sum_input_size += input_size;
			sum_output_size += output_size;
		}
	}

	log_summery(input_files, num_files, sum_input_size, output_name, sum_output_size);

	result = EXIT_SUCCESS;

cleanup:
	free(work_buf);
	add_airspace_suffix(NULL); /* free internal buffer */

	return result;
}


/**
 * @brief creates a file list from the input arguments
 *
 * Allocates memory for a file list. If n_file_names is zero, defaults to using
 * stdin as the input source. Handles "-" as a special case for stdin.
 *
 * @param argv			array of file names
 * @param argc			Number of file names (0 for stdin)
 * @param list_len		pointer to store the number of files in the list
 * @param is_reading_stdin	pointer to store whether stdin is being used
 *
 * @returns a pointer to the allocated file list
 */

static const char **allocate_file_list(char **argv, int argc, int *list_len, int *is_reading_stdin)
{
	const char **list;
	int i;

	assert(argv);
	assert(argc >= 0);
	assert(list_len);
	assert(is_reading_stdin);

	*is_reading_stdin = 0;

	*list_len = (argc == 0) ? 1 : argc;
	list = malloc_safe((size_t)*list_len * sizeof(*list));

	if (argc == 0) {
		list[0] = STD_IN_MARK;
		*is_reading_stdin = 1;
	} else {
		for (i = 0; i < argc; i++) {
			assert(argv[i]);
			if (!strcmp(argv[i], "-")) {
				list[i] = STD_IN_MARK;
				*is_reading_stdin = 1;
			} else {
				list[i] = argv[i];
			}
		}
	}

	return list;
}


static void print_usage(FILE *stream, const char *program_name)
{
	LOG_F(stream, "Usage: %s [OPTIONS...] [FILE... | -] [-o OUTPUT]\n", program_name);
	LOG_F(stream, "(De)compress AIRS science data FILE(s).\n\n");
	LOG_F(stream, "With no FILE, or when FILE is -, read standard input.\n");
	LOG_F(stream, "\nOptions:\n");
	LOG_F(stream, "  -c, --compress    Compress input files\n");
	LOG_F(stream, "  -o OUTPUT         Write output to OUTPUT\n");
	LOG_F(stream, "  -q, --quiet       Decrease verbosity\n");
	LOG_F(stream, "  -v, --verbose     Increase verbosity\n");
	LOG_F(stream, "  --[no]color       Print color codes in output\n");
	LOG_F(stream, "  -V, --version     Display version\n");
	LOG_F(stream, "  -h, --help        Display this help\n");
	LOG_F(stream, "\nExamples:\n");
	LOG_F(stream, "# Compressing files1 and files2 to output.air\n");
	LOG_F(stream, "airspace -c file1 file2 -o output.air\n");
	LOG_F(stream, "# Decompressing files (coming soon!)\n");
	LOG_F(stream, "airspace output.air -o file1.dat file2.dat\n");
}


static void print_version(void)
{
	if (log_get_level() < LOG_LEVEL_DEFAULT)
		LOG_STDOUT("%s\n", CMP_VERSION_STRING);
	else
		LOG_STDOUT(AIRSPACE_WELCOME_MESSAGE);
}


/**
 * @brief entry point for the AIRSPACE CLI tool
 *
 * @param argc	number of command-line arguments.
 * @param argv	array of command-line arguments.
 *
 * @returns EXIT_SUCCESS on success, EXIT_FAILURE on error
 */

int main(int argc, char *argv[])
{
	int return_val = EXIT_FAILURE;
	int ch;
	/*
	 * For long options that have no equivalent short option, use a
	 * non-character as a pseudo short option, starting with CHAR_MAX + 1.
	 */
	enum {
		STDOUT_OPT = CHAR_MAX + 1,
		COLOR_OPT,
		NO_COLOR_OPT,
		DEBUG_STDIN_CONSOLE_OPT,
		DEBUG_STDOUT_CONSOLE_OPT
	};
	static struct option long_options[] = {
		{ "compress",               no_argument,       NULL, 'c'                      },
		{ "params",                 required_argument, NULL, 'p'                      },
		{ "stdout",                 no_argument,       NULL, STDOUT_OPT               },
		{ "verbose",                no_argument,       NULL, 'v'                      },
		{ "quiet",                  no_argument,       NULL, 'q'                      },
		{ "color",                  no_argument,       NULL, COLOR_OPT                },
		{ "no-color",               no_argument,       NULL, NO_COLOR_OPT             },
		{ "version",                no_argument,       NULL, 'V'                      },
		{ "help",                   no_argument,       NULL, 'h'                      },
		{ "debug-stdin-is-consol",  no_argument,       NULL, DEBUG_STDIN_CONSOLE_OPT  },
		{ "debug-stdout-is-consol", no_argument,       NULL, DEBUG_STDOUT_CONSOLE_OPT },
		{ NULL,                     0,                 NULL, 0                        }
	};

	const char *program_name;
	const char **input_files = NULL;
	int num_files;
	int is_reading_stdin;

	/* Set defaults */
	enum operation_mode mode = MODE_DECOMPRESS;
	const char *output_filename = NULL;
	struct cmp_params params = { 0 };

	assert(argv);
	assert(argc >= 1);
	program_name = argv[0];
	log_setup_color();

	while ((ch = getopt_long(argc, argv, "Vvqhco:", long_options, NULL)) != -1) {
		switch (ch) {
		case 'c':
			mode = MODE_COMPRESS;
			break;
		case 'p':
			if (cmp_params_parse(optarg, &params) != CMP_PARSE_OK) {
				LOG_ERROR("Incorrect parameter option: %s", argv[optind-1]);
				return EXIT_FAILURE;
			}
			break;
		case 'o':
			output_filename = optarg;
			break;
		case STDOUT_OPT:
			output_filename = STD_OUT_MARK;
			break;
		case 'v':
			log_increase_verbosity();
			break;
		case 'q':
			log_decrease_verbosity();
			break;
		case COLOR_OPT:
			log_set_color(LOG_COLOR_ENABLED);
			break;
		case NO_COLOR_OPT:
			log_set_color(LOG_COLOR_DISABLED);
			break;
		case 'V':
			print_version();
			return EXIT_SUCCESS;
		case 'h':
			print_usage(stdout, program_name);
			return EXIT_SUCCESS;
		case DEBUG_STDIN_CONSOLE_OPT:
			util_force_stdin_console();
			break;
		case DEBUG_STDOUT_CONSOLE_OPT:
			util_force_stdout_console();
			break;
		default:
			print_usage(stderr, program_name);
			return EXIT_FAILURE;
		}
	}
	/*
	 * Adjust argc and argv to point to the remaining command-line arguments
	 * after the options have been processed
	 */
	argv += optind;
	argc -= optind;

	LOG_PLAIN(LOG_LEVEL_DEBUG, AIRSPACE_WELCOME_MESSAGE);

	input_files = allocate_file_list(argv, argc, &num_files, &is_reading_stdin);

	if (is_reading_stdin) {
		if (util_is_console(stdin)) {
			LOG_ERROR("stdin is a terminal, aborting");
			goto end;
		}
		LOG_DEBUG("Using stdin as an input");

		if (!output_filename) {
			if (util_is_console(stdout)) {
				LOG_ERROR("stdout is a terminal, aborting");
				goto end;
			}
			LOG_DEBUG("Using stdout as output");
			output_filename = STD_OUT_MARK;
		}
	}

	/* No info message by default when output is stdout */
	if (output_filename && !strcmp(output_filename, STD_OUT_MARK) &&
	    log_get_level() == LOG_LEVEL_DEFAULT)
		log_decrease_verbosity();

	/* Execute requested operation */
	switch (mode) {
	case MODE_COMPRESS:
		return_val = compress_file_list(output_filename, input_files, num_files, &params);
		break;
	case MODE_DECOMPRESS:
		LOG_ERROR("Decompression not implemented yet");
		break;
	default:
		LOG_ERROR("Invalid operation mode");
		break;
	}

end:
	free(input_files);

	return return_val;
}
