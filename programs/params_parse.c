/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Parsing and printing functions for the compression parameters
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "../lib/common/compiler.h"
#include "cmp.h"
#include "log.h"
#include "params_parse.h"
#include "../lib/common/arena.h"
#include "str_slice.h"


/* Represents a mapping from a string name to an integer value */
struct map_entry {
	struct s8 name;
	uint32_t value;
};
struct value_map {
	const struct map_entry *entries;
	size_t entry_count;
	const struct s8 *prefixes;
	size_t prefix_count;
};

static const struct map_entry preprocessing_entries[] = {
	{ S8("NONE"),  CMP_PREPROCESS_NONE  },
	{ S8("DIFF"),  CMP_PREPROCESS_DIFF  },
	{ S8("IWT"),   CMP_PREPROCESS_IWT   },
	{ S8("MODEL"), CMP_PREPROCESS_MODEL }
};
static const struct s8 preprocessing_prefixes[] = { S8("CMP_PREPROCESS_"), S8("CMP_"),
						    S8("PREPROCESS_") };
static const struct value_map preprocessing_map = {
	preprocessing_entries,
	ARRAY_SIZE(preprocessing_entries),
	preprocessing_prefixes,
	ARRAY_SIZE(preprocessing_prefixes),
};

static const struct map_entry encoder_type_entries[] = {
	{ S8("UNCOMPRESSED"), CMP_ENCODER_UNCOMPRESSED },
	{ S8("GOLOMB_ZERO"),  CMP_ENCODER_GOLOMB_ZERO  },
	{ S8("GOLOMB_MULTI"), CMP_ENCODER_GOLOMB_MULTI }
};
static const struct s8 encoder_type_prefixes[] = { S8("CMP_ENCODER_"), S8("CMP_"), S8("ENCODER_") };
static const struct value_map encoder_type_map = {
	encoder_type_entries,
	ARRAY_SIZE(encoder_type_entries),
	encoder_type_prefixes,
	ARRAY_SIZE(encoder_type_prefixes),
};

static const struct map_entry bool_entries[] = {
	{ S8("FALSE"), 0 },
	{ S8("TRUE"),  1 },
	{ S8("0"),     0 },
	{ S8("1"),     1 }
};
static const struct s8 bool_prefixes[] = { S8("CMP_") };
static const struct value_map bool_map = {
	bool_entries,
	ARRAY_SIZE(bool_entries),
	bool_prefixes,
	ARRAY_SIZE(bool_prefixes),
};

/* Helper macro for defining cmp_params struct fields */
#define PARAM_FIELD(f) offsetof(struct cmp_params, f), sizeof(((struct cmp_params *)0)->f)

/*
 * Dispatch table to map parameter names to their type and location within the
 * cmp_params struct.
 */
static const struct param_def {
	struct s8 name;
	size_t offset;
	size_t size;
	const struct value_map *value_map; /* NULL for uint32_t */
} param_keys[] = {
	/* Primary compression parameters */
	{ S8("primary_preprocessing"),         PARAM_FIELD(primary_preprocessing),         &preprocessing_map },
	{ S8("primary_encoder_type"),          PARAM_FIELD(primary_encoder_type),          &encoder_type_map  },
	{ S8("primary_encoder_param"),         PARAM_FIELD(primary_encoder_param),         NULL               },
	{ S8("primary_encoder_outlier"),       PARAM_FIELD(primary_encoder_outlier),       NULL               },
	{ S8("secondary_iterations"),          PARAM_FIELD(secondary_iterations),          NULL               },

	/* Secondary compression parameters */
	{ S8("secondary_preprocessing"),       PARAM_FIELD(secondary_preprocessing),       &preprocessing_map },
	{ S8("secondary_encoder_type"),        PARAM_FIELD(secondary_encoder_type),        &encoder_type_map  },
	{ S8("secondary_encoder_param"),       PARAM_FIELD(secondary_encoder_param),       NULL               },
	{ S8("secondary_encoder_outlier"),     PARAM_FIELD(secondary_encoder_outlier),     NULL               },
	{ S8("model_rate"),                    PARAM_FIELD(model_rate),                    NULL               },

	/* Feature flags */
	{ S8("checksum_enabled"),              PARAM_FIELD(checksum_enabled),              &bool_map          },
	{ S8("uncompressed_fallback_enabled"), PARAM_FIELD(uncompressed_fallback_enabled),
	 &bool_map                                                                                            }
};
#undef PARAM_FIELD


/* Write a value to a field in a cmp_params struct */
static void write_struct_field(struct cmp_params *params, const struct param_def *def, uint32_t val)
{
	void *addr = (uint8_t *)params + def->offset;

	switch (def->size) {
	case sizeof(uint8_t): {
		uint8_t u8 = (uint8_t)val;

		assert(u8 == val);
		memcpy(addr, &u8, def->size);
		break;
	}
	case sizeof(uint16_t): {
		uint16_t u16 = (uint16_t)val;

		assert(u16 == val);
		memcpy(addr, &u16, def->size);
		break;
	}
	case sizeof(uint32_t):
		memcpy(addr, &val, def->size);
		break;
	default:
		LOG_ERROR("This should never happen if the parameter definitions are correct");
		exit(EXIT_FAILURE);
	}
}


/* Read a value from a field in a cmp_params struct */
static uint32_t read_struct_field(const struct cmp_params *params, const struct param_def *def)
{
	const uint8_t *addr = (const uint8_t *)params + def->offset;

	switch (def->size) {
	case sizeof(uint8_t): {
		uint8_t val;

		memcpy(&val, addr, sizeof(val));
		return val;
	}
	case sizeof(uint16_t): {
		uint16_t val;

		memcpy(&val, addr, sizeof(val));
		return val;
	}
	case sizeof(uint32_t): {
		uint32_t val;

		memcpy(&val, addr, sizeof(val));
		return val;
	}
	default:
		LOG_ERROR("This should never happen if the parameter definitions are correct");
		exit(EXIT_FAILURE);
	}
}


static const struct param_def *find_param_definition(struct s8 key)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(param_keys); i++)
		if (s8_equals_ignore_case(key, param_keys[i].name))
			return &param_keys[i];

	return NULL;
}


static struct s8 s8_strip_prefixes_ignore_case(struct s8 s, const struct value_map *map)
{
	size_t i;

	for (i = 0; i < map->prefix_count; i++)
		if (s8_starts_with_ignore_case(s, map->prefixes[i]))
			return s8_skip(s, map->prefixes[i].len);
	return s;
}


static void log_valid_values(const struct param_def *def)
{
	size_t i;

	if (!def->value_map) {
		LOG_INFO("Hint: Value for '" PRIs8 "' must be a whole number.", S8_PARG(def->name));
		return;
	}

	LOG_INFO("Hint: Valid options for '" PRIs8 "' are:", S8_PARG(def->name));
	for (i = 0; i < def->value_map->entry_count; i++)
		LOG_INFO("  - '" PRIs8 "'", S8_PARG(def->value_map->entries[i].name));
}


static enum cmp_parse_status parse_value_str(const struct value_map *map, struct s8 value_str,
					     uint32_t *value_num)
{
	size_t i;

	*value_num = 0;

	if (map) {
		value_str = s8_strip_prefixes_ignore_case(value_str, map);
		for (i = 0; i < map->entry_count; i++) {
			if (s8_equals_ignore_case(value_str, map->entries[i].name)) {
				*value_num = map->entries[i].value;
				return CMP_PARSE_OK;
			}
		}
	} else { /* Handle uint32_t parameters */
		struct s8_u32_result result = s8_to_u32(value_str);

		if (result.ok) {
			*value_num = result.value;
			return CMP_PARSE_OK;
		}
	}
	return CMP_PARSE_INVALID_VALUE;
}


/* Parses a single "key=value" pair and updates the params struct */
static enum cmp_parse_status cmp_param_parse_kv_pair(struct s8 key, struct s8 value,
						     struct cmp_params *params)
{
	const struct param_def *key_def;
	uint32_t value_num;
	enum cmp_parse_status status;

	key = s8_trim(key);
	value = s8_trim(value);

	key_def = find_param_definition(key);
	if (!key_def) {
		LOG_ERROR("Unknown compression parameter: '" PRIs8 "'", S8_PARG(key));
		return CMP_PARSE_INVALID_KEY;
	}

	status = parse_value_str(key_def->value_map, value, &value_num);
	if (status == CMP_PARSE_OK) {
		write_struct_field(params, key_def, value_num);
	} else {
		LOG_ERROR("Invalid value '" PRIs8 "' for parameter '" PRIs8 "'.", S8_PARG(value),
			  S8_PARG(key));
		log_valid_values(key_def);
	}
	return status;
}


enum cmp_parse_status cmp_params_parse(const char *str, struct cmp_params *params)
{
	struct s8 remaining = s8_trim(s8_from_cstr(str));
	int saw_any = 0;

	while (remaining.len > 0) {
		enum cmp_parse_status r;
		struct s8 kv_pair;
		struct s8_split_result split;

		split = s8_split_at(remaining, ',');
		remaining = split.tail;
		kv_pair = s8_trim(split.head);
		if (!kv_pair.len)
			continue; /* Handles trailing or double commas */

		split = s8_split_at(kv_pair, '=');
		if (!split.ok) {
			LOG_ERROR("Parameters string is missing '=': '" PRIs8 "'.",
				  S8_PARG(kv_pair));
			return CMP_PARSE_MISSING_EQUAL;
		}

		r = cmp_param_parse_kv_pair(split.head, split.tail, params);
		if (r != CMP_PARSE_OK)
			return r;
		saw_any = 1;
	}

	if (!saw_any) {
		LOG_ERROR("Empty parameter string.");
		return CMP_PARSE_EMPTY_STR;
	}

	return CMP_PARSE_OK;
}


/* Concatenate string with decimal representation of uint32_t */
static struct s8 s8_concat_u32(struct arena *a, struct s8 head, uint32_t v)
{
	unsigned char b[10] = { 0 };
	ptrdiff_t i = S8_COUNTOF(b);

	do {
		b[--i] = '0' + (unsigned char)(v % 10);
	} while (v /= 10);

	return s8_concat(a, head, s8_span(&b[i], &b[S8_COUNTOF(b)]));
}


static struct s8 concat_value_as_string(struct arena *a, struct s8 head,
					const struct value_map *map, uint32_t value_num)
{
	static const struct s8 invalid = S8("INVALID");
	size_t i;

	if (!map)
		return s8_concat_u32(a, head, value_num);

	for (i = 0; i < map->entry_count; i++)
		if (value_num == map->entries[i].value)
			return s8_concat(a, head, map->entries[i].name);

	return s8_concat(a, head, invalid);
}


const char *cmp_params_to_string(struct arena *a, const struct cmp_params *par)
{
	static const struct s8 eq = S8(" = ");
	static const struct s8 new_line = S8(",\n");
	static const struct s8 end = S8("\n\0");
	struct s8 r = { 0 };
	size_t i;

	for (i = 0; i < ARRAY_SIZE(param_keys); i++) {
		const struct param_def *key_def = &param_keys[i];
		uint32_t numeric_val = 0;

		r = s8_concat(a, r, key_def->name);
		r = s8_concat(a, r, eq);

		numeric_val = read_struct_field(par, key_def);
		if (key_def->value_map == &bool_map)
			numeric_val = !!numeric_val; /* Normalize boolean values to 0 or 1 */

		r = concat_value_as_string(a, r, key_def->value_map, numeric_val);

		if (i < ARRAY_SIZE(param_keys) - 1)
			r = s8_concat(a, r, new_line);
	}

	r = s8_concat(a, r, end);

	return (const char *)r.s;
}
