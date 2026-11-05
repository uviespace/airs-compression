/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Lightweight string slice library for substring operations
 *
 * @warning A slice is a borrowed view and is only valid as long as the
 *          underlying buffer is valid. Do not return slices to stack-allocated
 *          memory from a function.
 *
 * @warning Encoding: this library is byte-oriented. Case-insensitive and
 *          whitespace helpers are ASCII-only (not Unicode/UTF-8 aware).
 *
 * This library is highly inspired by the string handling done in u-config by
 * @author Christopher Wellons (skeeto)
 */

#ifndef STR_SLICE_H_MIKG4GPN
#define STR_SLICE_H_MIKG4GPN

#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

/* clang-format off */
#if defined(__clang__) || defined(__GNUC__)
#  define S8_SUPPRESS_CAST_QUAL_PUSH                   \
     _Pragma("GCC diagnostic push")                    \
     _Pragma("GCC diagnostic ignored \"-Wcast-qual\"")
#  define S8_SUPPRESS_CAST_QUAL_POP \
      _Pragma("GCC diagnostic pop")
#else
#  define S8_SUPPRESS_CAST_QUAL_PUSH
#  define S8_SUPPRESS_CAST_QUAL_POP
#endif
/* clang-format on */


/** String slice type (8-bit chars) */
struct s8 {
	const unsigned char *s; /**< not null terminated string */
	ptrdiff_t len;          /**< length of string s */
};

/* Utility macros */
#define S8_COUNTOF(a) ((ptrdiff_t)(sizeof(a) / sizeof(*(a))))

/** Creates string slice from string literal @warning: do not use it with pointers! */
#define S8(s) { (const unsigned char *)(s), S8_COUNTOF(s) - 1 }

/**
 * printf format specifier for a string slice
 * Example: printf("my_s8: %" PRIs8 "\n", S8_PARG(my_s8));
 */
#define PRIs8      "%.*s"
/** Expands a string slice into the arguments required by PRIs8 */
#define S8_PARG(x) (int)(x).len, (const char *)(x).s

/** Create s8 string from C string */
static __inline struct s8 s8_from_cstr(const char *z);
/** Create s8 string from C string with known length */
static __inline struct s8 s8_make(const char *z, ptrdiff_t len);
/** Create s8 string from a pointer range [beg, end) */
static __inline struct s8 s8_span(const unsigned char *beg, const unsigned char *end);


/** Check if two s8 strings are equal */
static __inline int s8_equals(struct s8 s1, struct s8 s2);
/** Check if two s8 strings are equal, differences in case are ignored, ASCII-only (not UTF-8) */
static __inline int s8_equals_ignore_case(struct s8 s1, struct s8 s2);
/** Check if string starts with specified substring */
static __inline int s8_starts_with(struct s8 s, struct s8 pre);
static __inline int s8_starts_with_ignore_case(struct s8 s, struct s8 pre);
/** Check if string ends with specified substring */
static __inline int s8_ends_with(struct s8 s, struct s8 suf);
static __inline int s8_ends_with_ignore_case(struct s8 s, struct s8 suf);

/** Take head portion of string */
static __inline struct s8 s8_prefix(struct s8 s, ptrdiff_t len);
/** Remove head portion of string */
static __inline struct s8 s8_skip(struct s8 s, ptrdiff_t off);

/** Remove whitespace from the begin and the end */
static __inline struct s8 s8_trim(struct s8 s);
/** Remove prefix or empty string */
static __inline struct s8 s8_strip_prefix(struct s8 s, struct s8 pre);
/** Remove prefix or empty string, differences in case are ignored, ASCII-only */
static __inline struct s8 s8_strip_prefix_ignore_case(struct s8 s, struct s8 pre);
/** Remove suffix or empty string */
static __inline struct s8 s8_strip_suffix(struct s8 s, struct s8 suf);
/** Remove suffix  or empty string, differences in case are ignored, ASCII-only */
static __inline struct s8 s8_strip_suffix_ignore_case(struct s8 s, struct s8 suf);

/** split_at() parsing result */
struct s8_split_result {
	struct s8 head;
	struct s8 tail;
	int ok;
};
/** Split string at delimiter */
static __inline struct s8_split_result s8_split_at(struct s8 s, unsigned char delim);

/** s8_to_u32() parsing result */
struct s8_u32_result {
	uint32_t value;
	int ok;
};
/** Parse unsigned 32-bit integer from string */
static __inline struct s8_u32_result s8_to_u32(struct s8 s);

/** Arena-based string operations (requires arena.h) */
struct arena; /* Forward declaration */

/** Return a copy of a string using arena allocation */
static __inline struct s8 s8_clone(struct arena *a, struct s8 s);
/** Concatenate two strings using arena allocation */
static __inline struct s8 s8_concat(struct arena *a, struct s8 head, struct s8 tail);
/** Create a null-terminated C string from s8 using arena allocation */
static __inline char *s8_to_cstr(struct arena *a, struct s8 s);


static __inline struct s8 s8_from_cstr(const char *z)
{
	struct s8 r = { 0 };

	if (z) {
		size_t l = strlen(z);

		assert(l <= PTRDIFF_MAX);
		r.len = (ptrdiff_t)l;
		r.s = (const unsigned char *)z;
	}
	return r;
}

static __inline struct s8 s8_make(const char *z, ptrdiff_t len)
{
	struct s8 r = { 0 };

	if (len < 0)
		return r;

	r.s = (const unsigned char *)z;
	r.len = len;
	return r;
}

static __inline struct s8 s8_span(const unsigned char *beg, const unsigned char *end)
{
	struct s8 r = { 0 };

	assert(beg);
	assert(end);
	assert(end >= beg);
	r.s = beg;
	r.len = end - beg;
	return r;
}

/* Compare two byte arrays */
static int u8_compare(const unsigned char *s1, const unsigned char *s2, ptrdiff_t n)
{
	assert(n >= 0);
	return memcmp(s1, s2, (size_t)n);
}

static __inline int s8_equals(struct s8 s1, struct s8 s2)
{
	return s1.len == s2.len && !u8_compare(s1.s, s2.s, s1.len);
}

/* ASCII-only case conversion (not UTF-8) */
static __inline unsigned char u8_to_upper(unsigned char c)
{
	return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}

/* Compare two byte arrays, differences in case are ignored, ASCII-only (not UTF-8) */
static __inline int u8_compare_ignore_case(const unsigned char *a, const unsigned char *b,
					   ptrdiff_t n)
{
	for (; n; n--) {
		int d = u8_to_upper(*a++) - u8_to_upper(*b++);

		if (d)
			return d;
	}
	return 0;
}

static __inline int s8_equals_ignore_case(struct s8 a, struct s8 b)
{
	return a.len == b.len && !u8_compare_ignore_case(a.s, b.s, a.len);
}

static __inline int s8_starts_with(struct s8 s, struct s8 pre)
{
	return (s.len >= pre.len) && s8_equals(s8_prefix(s, pre.len), pre);
}

static __inline int s8_starts_with_ignore_case(struct s8 s, struct s8 pre)
{
	return (s.len >= pre.len) && s8_equals_ignore_case(s8_prefix(s, pre.len), pre);
}

static __inline int s8_ends_with(struct s8 s, struct s8 suf)
{
	return (s.len >= suf.len) && s8_equals(s8_skip(s, s.len - suf.len), suf);
}

static __inline int s8_ends_with_ignore_case(struct s8 s, struct s8 suf)
{
	return (s.len >= suf.len) && s8_equals_ignore_case(s8_skip(s, s.len - suf.len), suf);
}

static __inline struct s8 s8_prefix(struct s8 s, ptrdiff_t len)
{
	assert(len >= 0);
	assert(len <= s.len);
	s.len = len;
	return s;
}

static __inline struct s8 s8_skip(struct s8 s, ptrdiff_t off)
{
	assert(off >= 0);
	assert(off <= s.len);
	s.s += off;
	s.len -= off;
	return s;
}

static int u8_is_whitespace(unsigned char c)
{
	return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == '\f';
}

static __inline struct s8 s8_trim(struct s8 s)
{
	const unsigned char *p, *e;

	if (!s.len)
		return s;

	assert(s.s);
	p = s.s;
	e = s.s + s.len;

	while (p < e && u8_is_whitespace(*p))
		p++;
	while (e > p && u8_is_whitespace(*(e - 1)))
		e--;
	return s8_span(p, e);
}

static __inline struct s8 s8_strip_prefix(struct s8 s, struct s8 pre)
{
	struct s8 r = { 0 };

	if (s8_starts_with(s, pre))
		r = s8_skip(s, pre.len);
	return r;
}

static __inline struct s8 s8_strip_prefix_ignore_case(struct s8 s, struct s8 pre)
{
	struct s8 r = { 0 };

	if (s8_starts_with_ignore_case(s, pre))
		r = s8_skip(s, pre.len);
	return r;
}

static __inline struct s8 s8_strip_suffix(struct s8 s, struct s8 suf)
{
	struct s8 r = { 0 };

	if (s8_ends_with(s, suf))
		r = s8_prefix(s, s.len - suf.len);
	return r;
}

static __inline struct s8 s8_strip_suffix_ignore_case(struct s8 s, struct s8 suf)
{
	struct s8 r = { 0 };

	if (s8_ends_with_ignore_case(s, suf))
		r = s8_prefix(s, s.len - suf.len);
	return r;
}

static __inline struct s8_split_result s8_split_at(struct s8 s, unsigned char delim)
{
	struct s8_split_result r = { 0 };
	ptrdiff_t len = 0;

	for (len = 0; len < s.len; len++)
		if (s.s[len] == delim)
			break;

	if (len == s.len) {
		r.head = s;
		r.tail = s8_skip(s, s.len);
		return r;
	}
	r.head = s8_prefix(s, len);
	r.tail = s8_skip(s, len + 1);
	r.ok = 1;
	return r;
}

static __inline struct s8_u32_result s8_to_u32(struct s8 s)
{
	struct s8_u32_result r = { 0 };
	uint32_t value = 0;
	ptrdiff_t i = 0;

	if (s.len == 0)
		return r;
	if (s.len > 1 && s.s[0] == '0') /* reject leading 0 */
		return r;

	for (; i < s.len; i++) {
		uint32_t const d = (uint32_t)(s.s[i] - '0');

		if (d > 9)
			return r; /* Not a digit */
		if (value > (UINT32_MAX - d) / 10)
			return r;
		value = (value * 10) + d;
	}

	r.value = value;
	r.ok = 1;
	return r;
}


/* Arena-based string operations */
#include "../lib/common/arena.h"

static __inline struct s8 s8_clone(struct arena *a, struct s8 s)
{
	struct s8 r = { 0 };
	unsigned char *clone;

	assert(s.len >= 0);

	clone = ARENA_NEW_ARRAY(a, s.len, unsigned char);
	if (s.s)
		memcpy(clone, s.s, (size_t)s.len);
	r.s = clone;
	r.len = s.len;
	return r;
}

static __inline struct s8 s8_concat(struct arena *a, struct s8 head, struct s8 tail)
{
	struct s8 r = { 0 };
	unsigned char *combined;

	assert(head.len >= 0);
	assert(tail.len >= 0);

	if (head.len > PTRDIFF_MAX - tail.len)
		arena_oom();
	r.len = head.len + tail.len;

	if (arena_buf_ends_at_top(*a, head.s, head.len)) {
		arena_grow_last(a, head.s, head.len, r.len);
		/*
		 * Casting away const is "safe" here: as we only write the newly
		 * grown tail.
		 */
		S8_SUPPRESS_CAST_QUAL_PUSH
		combined = (unsigned char *)head.s;
		S8_SUPPRESS_CAST_QUAL_POP
	} else {
		combined = ARENA_NEW_ARRAY(a, r.len, unsigned char);
		if (head.s)
			memcpy(combined, head.s, (size_t)head.len);
	}

	if (tail.s)
		memcpy(combined + head.len, tail.s, (size_t)tail.len);

	r.s = combined;
	return r;
}

static __inline char *s8_to_cstr(struct arena *a, struct s8 s)
{
	char *cstr;

	cstr = ARENA_NEW_ARRAY(a, s.len + 1, char);
	if (s.s)
		memcpy(cstr, s.s, (size_t)s.len);
	cstr[s.len] = '\0';
	return cstr;
}

#endif /* STR_SLICE_H_MIKG4GPN */
