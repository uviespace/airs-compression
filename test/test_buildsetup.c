/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief build environment tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unity.h>

#include "../lib/cmp.h"

#ifndef CMP_MESON_BUILD_ROOT
#  define CMP_MESON_BUILD_ROOT "."
#endif


void test_version_matches(void)
{
	enum { MAX_PATH_SIZE = 256, CONTENT_SIZE = 200 };
	char file_path[256] = { 0 };
	char content[CONTENT_SIZE] = { 0 };
	const char *version_key = "\"version\": \"";

	FILE *fp;
	size_t ret;

	int read = snprintf(file_path, sizeof(file_path), "%s/meson-info/intro-projectinfo.json",
			    CMP_MESON_BUILD_ROOT);

	TEST_ASSERT_NOT_EQUAL(0, read);
	TEST_ASSERT_LESS_THAN(sizeof(file_path), read);

	fp = fopen(file_path, "r");
	TEST_ASSERT_NOT_NULL_MESSAGE(fp, file_path);

	ret = fread(content, sizeof(content[0]), CONTENT_SIZE, fp);
	TEST_ASSERT_GREATER_THAN(strlen(version_key), ret);

	{
		char *end = NULL;
		char version_exp[10];
		char *version = strstr(content, version_key);

		TEST_ASSERT_NOT_NULL(version);
		version += strlen(version_key);

		end = strchr(version, '\"');
		TEST_ASSERT_NOT_NULL(end);
		*end = '\0';

		snprintf(version_exp, sizeof(version_exp), "%d.%d.%d", CMP_VERSION_MAJOR,
			 CMP_VERSION_MINOR, CMP_VERSION_RELEASE);
		TEST_ASSERT_EQUAL_STRING(version_exp, version);
	}
	fclose(fp);
}
