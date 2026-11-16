/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Test configuration parser and serializes functionality
 */

#include <string.h>
#include <unity.h>

#include "test_common.h"

/* Compile the implementation with the test arena hooks already configured. */
#include "../programs/params_parse.c"


void test_parse_preprocess_enums(void)
{
	static const struct {
		const char *name;
		uint32_t value;
	} preprocess_cases[] = {
		{ "NONE",                CMP_PREPROCESS_NONE  },
		{ "DIFF",                CMP_PREPROCESS_DIFF  },
		{ "IWT",                 CMP_PREPROCESS_IWT   },
		{ "MODEL",               CMP_PREPROCESS_MODEL },
		{ "DiFf",                CMP_PREPROCESS_DIFF  },
		{ "PREPROCESS_DIFF",     CMP_PREPROCESS_DIFF  },
		{ "CMP_PREPROCESS_DIFF", CMP_PREPROCESS_DIFF  },
		{ "CMP_DIFF",            CMP_PREPROCESS_DIFF  },
		{ "CmP_pRePrOcEsS_dIfF", CMP_PREPROCESS_DIFF  }
	};

	size_t i;
	struct arena *a = clear_test_arena();
	unsigned int const s_size = 64;
	char *s = ARENA_NEW_ARRAY(a, s_size, char);
	struct cmp_params par, par_exp;
	enum cmp_parse_status status;

	for (i = 0; i < ARRAY_SIZE(preprocess_cases); i++) {
		memset(&par_exp, 0xff, sizeof(par_exp));
		par_exp.primary_preprocessing = preprocess_cases[i].value;
		snprintf(s, s_size, "primary_preprocessing=%s", preprocess_cases[i].name);
		memset(&par, 0xff, sizeof(par));

		status = cmp_params_parse(s, &par);

		TEST_ASSERT_EQUAL_MESSAGE(CMP_PARSE_OK, status, s);
		TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&par_exp, &par, sizeof(par_exp), s);
	}

	for (i = 0; i < ARRAY_SIZE(preprocess_cases); i++) {
		memset(&par_exp, 0xff, sizeof(par_exp));
		par_exp.secondary_preprocessing = preprocess_cases[i].value;
		snprintf(s, s_size, "secondary_preprocessing=%s", preprocess_cases[i].name);
		memset(&par, 0xff, sizeof(par));

		status = cmp_params_parse(s, &par);

		TEST_ASSERT_EQUAL_MESSAGE(CMP_PARSE_OK, status, s);
		TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&par_exp, &par, sizeof(par_exp), s);
	}
}


void test_parse_encoder_types_enums(void)
{
	static const struct {
		const char *name;
		uint32_t value;
	} encoder_cases[] = {
		{ "UNCOMPRESSED",             CMP_ENCODER_UNCOMPRESSED },
		{ "GOLOMB_ZERO",              CMP_ENCODER_GOLOMB_ZERO  },
		{ "GOLOMB_MULTI",             CMP_ENCODER_GOLOMB_MULTI },
		{ "ENCODER_UNCOMPRESSED",     CMP_ENCODER_UNCOMPRESSED },
		{ "CMP_ENCODER_UNCOMPRESSED", CMP_ENCODER_UNCOMPRESSED },
		{ "CMP_UNCOMPRESSED",         CMP_ENCODER_UNCOMPRESSED },
		{ "CmP_EnCoDeR_uNcOmPrEsSeD", CMP_ENCODER_UNCOMPRESSED }
	};

	size_t i;
	struct arena *a = clear_test_arena();
	unsigned int const s_size = 64;
	char *s = ARENA_NEW_ARRAY(a, s_size, char);
	struct cmp_params par, par_exp;
	enum cmp_parse_status status;

	for (i = 0; i < ARRAY_SIZE(encoder_cases); i++) {
		memset(&par_exp, 0xff, sizeof(par_exp));
		par_exp.primary_encoder_type = encoder_cases[i].value;
		snprintf(s, s_size, "primary_encoder_type=%s", encoder_cases[i].name);
		memset(&par, 0xff, sizeof(par));

		status = cmp_params_parse(s, &par);

		TEST_ASSERT_EQUAL_MESSAGE(CMP_PARSE_OK, status, s);
		TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&par_exp, &par, sizeof(par_exp), s);
	}

	for (i = 0; i < ARRAY_SIZE(encoder_cases); i++) {
		memset(&par_exp, 0xff, sizeof(par_exp));
		par_exp.secondary_encoder_type = encoder_cases[i].value;
		snprintf(s, s_size, "secondary_encoder_type=%s", encoder_cases[i].name);
		memset(&par, 0xff, sizeof(par));

		status = cmp_params_parse(s, &par);

		TEST_ASSERT_EQUAL_MESSAGE(CMP_PARSE_OK, status, s);
		TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&par_exp, &par, sizeof(par_exp), s);
	}
}


void test_parse_boolean_types(void)
{
	static const struct {
		const char *name;
		uint8_t value;
	} boolean_cases[] = {
		{ "TRUE",      1 },
                { "FALSE",     0 },
                { "1",         1 },
                { "0",         0 },
		{ "CMP_TRUE",  1 },
                { "CMP_FALSE", 0 },
                { "Cmp_True",  1 },
                { "Cmp_False", 0 }
	};

	size_t i;
	struct arena *a = clear_test_arena();
	unsigned int const s_size = 64;
	char *s = ARENA_NEW_ARRAY(a, s_size, char);
	struct cmp_params par, par_exp;
	enum cmp_parse_status status;

	for (i = 0; i < ARRAY_SIZE(boolean_cases); i++) {
		memset(&par_exp, 0xff, sizeof(par_exp));
		par_exp.checksum_enabled = boolean_cases[i].value;
		snprintf(s, s_size, "checksum_enabled=%s", boolean_cases[i].name);
		memset(&par, 0xff, sizeof(par));

		status = cmp_params_parse(s, &par);

		TEST_ASSERT_EQUAL_MESSAGE(CMP_PARSE_OK, status, s);
		TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&par_exp, &par, sizeof(par_exp), s);
	}
}


void test_parse_numeric_value_zero(void)
{
	enum cmp_parse_status status;
	struct cmp_params par = { 0 };
	struct cmp_params par_exp = { 0 };

	par.primary_encoder_param = ~0U;
	par_exp.primary_encoder_param = 0;

	status = cmp_params_parse("primary_encoder_param=0", &par);

	TEST_ASSERT_EQUAL(CMP_PARSE_OK, status);
	TEST_ASSERT_EQUAL_MEMORY(&par_exp, &par, sizeof(par_exp));
}


void test_parse_typical_numeric_value(void)
{
	enum cmp_parse_status status;
	struct cmp_params par = { 0 };
	struct cmp_params par_exp = { 0 };

	par_exp.primary_encoder_param = 42;

	status = cmp_params_parse("primary_encoder_param=42", &par);

	TEST_ASSERT_EQUAL(CMP_PARSE_OK, status);
	TEST_ASSERT_EQUAL_MEMORY(&par_exp, &par, sizeof(par_exp));
}


void test_parse_maximum_numeric_value(void)
{
	enum cmp_parse_status status;
	struct cmp_params par = { 0 };
	struct cmp_params par_exp = { 0 };

	par_exp.primary_encoder_param =
		maximum_unsigned_value_of_type(par_exp.primary_encoder_param);

	status = cmp_params_parse("primary_encoder_param=4294967295", &par);

	TEST_ASSERT_EQUAL(CMP_PARSE_OK, status);
	TEST_ASSERT_EQUAL_MEMORY(&par_exp, &par, sizeof(par_exp));
}


void test_use_last_if_same_key_twice(void)
{
	enum cmp_parse_status status;
	struct cmp_params par = { 0 };
	struct cmp_params par_exp = { 0 };

	par_exp.primary_encoder_param = 42;

	status = cmp_params_parse("primary_encoder_param=23,primary_encoder_param=42", &par);

	TEST_ASSERT_EQUAL(CMP_PARSE_OK, status);
	TEST_ASSERT_EQUAL_MEMORY(&par_exp, &par, sizeof(par_exp));
}


void test_trailing_comma_is_allowed(void)
{
	enum cmp_parse_status status;
	struct cmp_params par = { 0 };
	struct cmp_params par_exp = { 0 };

	par_exp.primary_preprocessing = CMP_PREPROCESS_MODEL;

	status = cmp_params_parse(",primary_preprocessing=CMP_PREPROCESS_MODEL,", &par);

	TEST_ASSERT_EQUAL(CMP_PARSE_OK, status);
	TEST_ASSERT_EQUAL_MEMORY(&par_exp, &par, sizeof(par_exp));
}


void test_whitespace_is_allowed(void)
{
	enum cmp_parse_status status;
	struct cmp_params par = { 0 };
	struct cmp_params par_exp = { 0 };

	par_exp.primary_preprocessing = CMP_PREPROCESS_MODEL;

	status = cmp_params_parse(" primary_preprocessing\t = CMP_PREPROCESS_MODEL\n", &par);

	TEST_ASSERT_EQUAL(CMP_PARSE_OK, status);
	TEST_ASSERT_EQUAL_MEMORY(&par_exp, &par, sizeof(par_exp));
}


void test_keys_are_case_insensitive(void)
{
	enum cmp_parse_status status;
	struct cmp_params par = { 0 };
	struct cmp_params par_exp = { 0 };

	par_exp.primary_encoder_param = 42;

	status = cmp_params_parse("PrImArY_EnCoDeR_pArAm=42", &par);

	TEST_ASSERT_EQUAL(CMP_PARSE_OK, status);
	TEST_ASSERT_EQUAL_MEMORY(&par_exp, &par, sizeof(par_exp));
}


void test_parse_all_compression_parameters(void)
{
	/* arrange */
	enum cmp_parse_status status;
	struct cmp_params par = { 0 };
	struct cmp_params par_exp = { 0 };
	static const char *str = { "primary_preprocessing = IWT,"
				   "primary_encoder_type = GOLOMB_MULTI,"
				   "primary_encoder_param = 12,"
				   "primary_encoder_outlier = 0,"

				   "secondary_iterations = 4294967295,"
				   "secondary_preprocessing = DIFF,"
				   "secondary_encoder_type = GOLOMB_ZERO,"
				   "secondary_encoder_param = 42,"
				   "secondary_encoder_outlier = 1,"
				   "model_rate = 16,"

				   "checksum_enabled = FALSE,"
				   "uncompressed_fallback_enabled = TRUE," };

	par_exp.primary_preprocessing = CMP_PREPROCESS_IWT;
	par_exp.primary_encoder_type = CMP_ENCODER_GOLOMB_MULTI;
	par_exp.primary_encoder_param = 12;
	par_exp.primary_encoder_outlier = 0;

	par_exp.secondary_iterations = UINT32_MAX;
	par_exp.secondary_preprocessing = CMP_PREPROCESS_DIFF;
	par_exp.secondary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	par_exp.secondary_encoder_param = 42;
	par_exp.secondary_encoder_outlier = 1;
	par_exp.model_rate = 16;

	par_exp.checksum_enabled = 0;
	par_exp.uncompressed_fallback_enabled = 1;

	/* act */
	status = cmp_params_parse(str, &par);

	/* assert */
	TEST_ASSERT_EQUAL(CMP_PARSE_OK, status);
	TEST_ASSERT_EQUAL_MEMORY(&par_exp, &par, sizeof(par_exp));
}


void test_detect_empty_string(void)
{
	size_t i;
	static const char *const str[] = {
		"", " ", "\t", "\r", "\n", ",", ", ,",
	};

	for (i = 0; i < ARRAY_SIZE(str); i++) {
		struct cmp_params par;

		enum cmp_parse_status status = cmp_params_parse(str[i], &par);

		TEST_ASSERT_EQUAL_INT(CMP_PARSE_EMPTY_STR, status);
	}
}


void test_detect_str_is_NULL(void)
{
	struct cmp_params par;

	enum cmp_parse_status status = cmp_params_parse(NULL, &par);

	TEST_ASSERT_EQUAL_INT(CMP_PARSE_EMPTY_STR, status);
}


void test_detects_invalid_syntax_missing_equals(void)
{
	size_t i;
	static const char *const str[] = {
		"primary_preprocessing CMP_PREPROCESS_MODEL",
		"primary_preprocessing CMP_PREPROCESS_MODEL,",
		"primary_preprocessingCMP_PREPROCESS_MODEL",
	};

	for (i = 0; i < ARRAY_SIZE(str); i++) {
		struct cmp_params par;

		enum cmp_parse_status status = cmp_params_parse(str[i], &par);

		TEST_ASSERT_EQUAL_INT_MESSAGE(CMP_PARSE_MISSING_EQUAL, status, str[i]);
	}
}

void test_detect_invalid_numeric_values(void)
{
	size_t i;
	static const char *const str[] = {
		"primary_encoder_param=4294967296", /* UINT32_MAX + 1 */
		"primary_encoder_param=02",         "primary_encoder_param=000000000002",
		"primary_encoder_param=2.2",        "primary_encoder_param=2.",
		"primary_encoder_param=.2",         "primary_encoder_param=2 2",
		"primary_encoder_param=-2",         "primary_encoder_param=0x2",
		"primary_encoder_param=a",          "primary_encoder_param=",
	};

	for (i = 0; i < ARRAY_SIZE(str); i++) {
		struct cmp_params par;

		enum cmp_parse_status status = cmp_params_parse(str[i], &par);

		TEST_ASSERT_EQUAL_INT_MESSAGE(CMP_PARSE_INVALID_VALUE, status, str[i]);
	}
}


void test_detect_invalid_enum_keys(void)
{
	size_t i;
	static const char *const str[] = {
		"primary_preprocessing=",      "primary_preprocessing=,",
		"primary_preprocessing=1",     "primary_preprocessing=DIF",
		"primary_preprocessing==DIFF", "primary_preprocessing=DIF F",
	};

	for (i = 0; i < ARRAY_SIZE(str); i++) {
		struct cmp_params par;

		enum cmp_parse_status status = cmp_params_parse(str[i], &par);

		TEST_ASSERT_EQUAL_INT_MESSAGE(CMP_PARSE_INVALID_VALUE, status, str[i]);
	}
}


void test_detect_invalid_keys(void)
{
	struct cmp_params par;

	enum cmp_parse_status status = cmp_params_parse("INVALID=3", &par);

	TEST_ASSERT_EQUAL_INT(CMP_PARSE_INVALID_KEY, status);
}

void test_detect_no_keys(void)
{
	struct cmp_params par;

	enum cmp_parse_status status = cmp_params_parse("=3", &par);

	TEST_ASSERT_EQUAL_INT(CMP_PARSE_INVALID_KEY, status);
}


void test_stringify_all_parameters(void)
{
	struct arena *a = clear_test_arena();
	struct cmp_params par = { 0 };
	const char *str;

	/* arrange */
	par.primary_preprocessing = (enum cmp_preprocessing)(-1);
	par.primary_encoder_type = CMP_ENCODER_GOLOMB_MULTI;
	par.primary_encoder_param = 12;

	par.secondary_iterations = UINT32_MAX;
	par.secondary_preprocessing = CMP_PREPROCESS_DIFF;
	par.secondary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	par.secondary_encoder_param = 42;
	par.secondary_encoder_outlier = 1;
	par.model_rate = 16;

	par.checksum_enabled = 0;
	par.uncompressed_fallback_enabled = 1;

	/* act */
	str = cmp_params_to_string(a, &par);

	/* assert */
	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "primary_preprocessing = INVALID,"), str);
	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "primary_encoder_type = GOLOMB_MULTI,"), str);
	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "primary_encoder_param = 12,"), str);
	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "primary_encoder_outlier = 0,"), str);

	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "secondary_iterations = 4294967295,"), str);
	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "secondary_preprocessing = DIFF,"), str);
	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "secondary_encoder_type = GOLOMB_ZERO,"), str);
	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "secondary_encoder_param = 42,"), str);
	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "secondary_encoder_outlier = 1,"), str);
	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "model_rate = 16,"), str);

	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "checksum_enabled = FALSE,"), str);
	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "uncompressed_fallback_enabled = TRUE\n"), str);
	/* no ',' on last line*/
}


void test_to_string_bools_are_normalized(void)
{
	struct arena *a = clear_test_arena();
	struct cmp_params par = { 0 };
	const char *s;

	par.checksum_enabled = 42;

	s = cmp_params_to_string(a, &par);

	TEST_ASSERT_TRUE_MESSAGE(strstr(s, "checksum_enabled = TRUE"), s);
}


void test_stringify_invalid_enum_values(void)
{
	struct arena *a = clear_test_arena();
	struct cmp_params par = { 0 };
	const char *str;

	par.primary_preprocessing = (enum cmp_preprocessing)(-1);
	par.primary_encoder_type = (enum cmp_encoder_type)(-2);
	par.secondary_preprocessing = (enum cmp_preprocessing)(-3);
	par.secondary_encoder_type = (enum cmp_encoder_type)(-4);

	str = cmp_params_to_string(a, &par);

	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "primary_preprocessing = INVALID"), str);
	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "primary_encoder_type = INVALID"), str);
	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "secondary_preprocessing = INVALID"), str);
	TEST_ASSERT_TRUE_MESSAGE(strstr(str, "secondary_encoder_type = INVALID"), str);
}


void test_to_string_parse_roundtrip(void)
{
	struct arena *arena = clear_test_arena();
	struct cmp_params a = { 0 };
	struct cmp_params b = { 0 };
	const char *str;
	enum cmp_parse_status status;

	a.primary_preprocessing = CMP_PREPROCESS_NONE;
	a.primary_encoder_type = CMP_ENCODER_GOLOMB_MULTI;
	a.primary_encoder_param = 12;
	a.primary_encoder_outlier = 0;
	a.secondary_iterations = UINT32_MAX;
	a.secondary_preprocessing = CMP_PREPROCESS_DIFF;
	a.secondary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	a.secondary_encoder_param = 42;
	a.secondary_encoder_outlier = 1;
	a.model_rate = 16;
	a.checksum_enabled = 0;
	a.uncompressed_fallback_enabled = 1;

	str = cmp_params_to_string(arena, &a);
	status = cmp_params_parse(str, &b);

	TEST_ASSERT_EQUAL_INT_MESSAGE(CMP_PARSE_OK, status, str);
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&a, &b, sizeof(a), str);
}
