/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Big-Endian Bitstream Writer
 *
 * Usage:
 * - Initialise the bitstream writer:
 *        error_code = bitstream_writer_init();
 * - Write bits to the bitstream:
 *        bitstream_add_XXX();
 * - Flush remaining bits to the buffer:
 *        bytes_written_or_error_code = bitstream_flush();
 */

#ifndef CMP_BITSTREAM_WRITER_H
#define CMP_BITSTREAM_WRITER_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "../cmp.h"
#include "../common/err_private.h"
#include "../common/byteorder.h"
#include "../common/bithacks.h"

#ifdef __LITTLE_ENDIAN
#  define BITSTREAM_IS_CPU_LITTLE_ENDIAN 1
#elif defined(__BIG_ENDIAN)
#  define BITSTREAM_IS_CPU_LITTLE_ENDIAN 0
#endif

/*
 * GCC/BCC on 32-bit SPARC may split these 64-bit accesses into 32-bit
 * operations at -O2, which in turn tends to make the back end emit ld/st
 * instead of ldd/std.
 * Keep the aligned pointers volatile to preserve opaque 64-bit accesses
 * and reliably get ldd/std. Remove this once the compiler handles 64-bit
 * SPARC32 operations correctly.
 */
#if defined(__GNUC__) && !defined(__clang__) && defined(__sparc__) && !defined(__sparc_v9__)
#  define SPARC_VOLATILE_GCC volatile
#else
#  define SPARC_VOLATILE_GCC
#endif

/**
 * @brief This structure maintains the state of the bitstream writer
 *
 * @warning This structure MUST NOT be directly manipulated by external code.
 *	Always use the provided API functions to interact with the structure.
 */
struct bitstream_writer {
	uint64_t cache;       /**< Local bit cache  */
	unsigned int bit_cap; /**< Bit capacity left in the cache */
	uint8_t *start;       /**< Beginning of bitstream */
	uint8_t *ptr;         /**< Current write position */
	uint8_t *end;         /**< End of the bitstream pointer */
	uint32_t error;       /**< Sticky error code */
};


/**
 * @brief Initialise a bitstream writer
 *
 * @param bs	pointer to an already allocated bitstream_writer structure
 * @param dst	start address of the bitstream buffer; has to be 8-byte aligned
 * @param size	capacity of the bitstream buffer in bytes
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

static __inline uint32_t bitstream_writer_init(struct bitstream_writer *bs, void *dst,
					       uint32_t size)
{
	if (!bs)
		return CMP_ERROR(INT_BITSTREAM);
	memset(bs, 0, sizeof(*bs));

	if (!dst)
		return bs->error = CMP_ERROR(DST_NULL);
	if (!is_aligned(dst, CMP_DST_ALIGNMENT))
		return bs->error = CMP_ERROR(DST_UNALIGNED);

	bs->cache = 0;
	bs->bit_cap = 64;
	bs->start = dst;
	bs->ptr = dst;
	bs->end = (uint8_t *)dst + size;
	return bs->error = CMP_ERROR(NO_ERROR);
}


/**
 * @brief Stores a 64-bit integer as big-endian bytes
 *
 * @param ptr	8 byte aligned address to write
 * @param val	value to write
 */

static __inline void put_be64_aligned(void *ptr, uint64_t val)
{
	val = cpu_to_be64(val);
#if defined(__GNUC__) || defined(__clang__)
	{
		typedef uint64_t __attribute__((may_alias)) aliased_u64;
		*(SPARC_VOLATILE_GCC aliased_u64 *)ptr = val;
	}
#else
	memcpy(ptr, &val, sizeof(val));
#endif
}


/**
 * @brief Stores a 32-bit integer as big-endian bytes
 *
 * @param ptr	4 byte aligned address to write
 * @param val	value to write
 */

static __inline void put_be32_aligned(void *ptr, uint32_t val)
{
	val = cpu_to_be32(val);
#if defined(__GNUC__) || defined(__clang__)
	{
		typedef uint32_t __attribute__((may_alias)) aliased_u32;
		*(aliased_u32 *)ptr = val;
	}
#else
	memcpy(ptr, &val, sizeof(val));
#endif
}


/**
 * @brief Stores a 16-bit integer as big-endian bytes
 *
 * @param ptr	2 byte aligned address to write
 * @param val	value to write
 */

static __inline void put_be16_aligned(void *ptr, uint16_t val)
{
	val = cpu_to_be16(val);
#if defined(__GNUC__) || defined(__clang__)
	{
		typedef uint16_t __attribute__((may_alias)) aliased_u16;
		*(aliased_u16 *)ptr = val;
	}
#else
	memcpy(ptr, &val, sizeof(val));
#endif
}


/**
 * @brief Get the current error status of the bitstream writer
 *
 * @returns the first occurred error code, which can be checked using cmp_is_error()
 */

static __inline uint32_t bitstream_error(const struct bitstream_writer *bs)
{
	if (!bs)
		return CMP_ERROR(INT_BITSTREAM);
	return bs->error;
}


/**
 * @brief Adds up to 32 bits to the bitstream
 *
 * @note This function writes bits into an internal cache, which is only flushed to
 *	 the output buffer when full or when bitstream_flush() is explicitly called.
 *	 As a result, after completing a sequence of writes, the caller **must** call
 *	 bitstream_flush() to ensure all bits are properly written to the buffer.
 * @note This function uses sticky error handling. Once an error occurs, subsequent
 *	 calls are ignored. Possible error conditions can be tested with
 *	 bitstream_error() or bitstream_flush().
 *
 * @param bs		pointer to an initialised bitstream_writer structure
 * @param value		bits to write to the bitstream; must be "clean", meaning
 *			all high bits above nbBits are 0
 * @param nb_bits	number of bits to write from value; must be <= 32
 */

static __inline void bitstream_add_bits32(struct bitstream_writer *bs, uint32_t value,
					  unsigned int nb_bits)
{
	if (cmp_is_error_int(bitstream_error(bs)))
		return;

	if (nb_bits > 32) {
		bs->error = CMP_ERROR(INT_BITSTREAM);
		return;
	}
	if (nb_bits < 32 && (value >> nb_bits)) {
		bs->error = CMP_ERROR(INT_BITSTREAM);
		return;
	}

	/* Fast path: bits fit in current cache */
	if (nb_bits < bs->bit_cap) {
		bs->cache = (bs->cache << nb_bits) | value;
		bs->bit_cap -= nb_bits;
		return;
	}

	/* Slow path: need to flush cache */
	if (bs->end - bs->ptr >= 8) {
		bs->cache <<= bs->bit_cap;
		bs->cache |= value >> (nb_bits - bs->bit_cap);
		put_be64_aligned(bs->ptr, bs->cache);

		bs->ptr += 8;
		bs->cache = value;
		bs->bit_cap += 64 - nb_bits;
	} else {
		bs->error = CMP_ERROR(DST_TOO_SMALL);
	}
}


/**
 * @brief Add a byte array to the bitstream
 *
 * @note This function only works after previous bit writes ended on a byte
 *	 boundary
 * @note This function uses sticky error handling. Once an error occurs, subsequent
 *	 calls are ignored. Possible error conditions can be tested with
 *	 bitstream_error() or bitstream_flush().
 *
 * @param bs		pointer to initialised bitstream_writer
 * @param src8		source buffer of bytes
 * @param nb_bytes	number of bytes to add
 *
 */

static __inline void bitstream_add_bytes(struct bitstream_writer *bs, const uint8_t *src8,
					 uint32_t nb_bytes)
{
	uint32_t i, l8;
	size_t remaining, cached_bytes;

	if (cmp_is_error_int(bitstream_error(bs)))
		return;

	if (!is_multiple_of(bs->bit_cap, 8)) {
		bs->error = CMP_ERROR(INT_BITSTREAM);
		return;
	}

	if (!src8) {
		bs->error = CMP_ERROR(INT_BITSTREAM);
		return;
	}

	remaining = (size_t)(bs->end - bs->ptr);
	cached_bytes = sizeof(bs->cache) - ((bs->bit_cap + 7) / 8);
	if (cached_bytes > remaining || nb_bytes > remaining - cached_bytes) {
		bs->error = CMP_ERROR(DST_TOO_SMALL);
		return;
	}

	for (i = 0; i < nb_bytes && bs->bit_cap != 64; i++)
		bitstream_add_bits32(bs, src8[i], 8);

	/* Assume bitstream is flushed when bs->bit_cap == 64 is hit */
	l8 = (nb_bytes - i) & ~7U;
	memcpy(bs->ptr, &src8[i], l8);
	i += l8;
	bs->ptr += l8;

	for (; i < nb_bytes; i++)
		bitstream_add_bits32(bs, src8[i], 8);
}


/**
 * @brief Write an array of 16-bit values as big-endian to the bitstream
 *
 * @note This function only works after previous bit writes ended on a 64-bit
 *	 boundary; call bitstream_flush() first if needed
 * @note This function uses sticky error handling. Once an error occurs, subsequent
 *	 calls are ignored. Possible error conditions can be tested with
 *	 bitstream_error() or bitstream_flush().
 *
 * @param bs		pointer to initialised bitstream_writer
 * @param src16		source buffer of 16-bit values (native endianness)
 * @param nb_samples	number of samples to write
 */

static __inline void bitstream_add_be16_array(struct bitstream_writer *bs, const int16_t *src16,
					      uint32_t nb_samples)
{
	uint32_t i = 0;
	uint8_t *dst;

	if (cmp_is_error_int(bitstream_error(bs)))
		return;

	if (bs->bit_cap != 64) {
		bs->error = CMP_ERROR(INT_BITSTREAM);
		return;
	}

	if (!src16) {
		bs->error = CMP_ERROR(INT_BITSTREAM);
		return;
	}

	if (!is_aligned(bs->ptr, CMP_DST_ALIGNMENT)) {
		bs->error = CMP_ERROR(INT_BITSTREAM);
		return;
	}

	if (nb_samples > (size_t)(bs->end - bs->ptr) / sizeof(int16_t)) {
		bs->error = CMP_ERROR(DST_TOO_SMALL);
		return;
	}

	/*
	 * The bulk copy below is tuned for the GR712RC (SPARC V8/LEON3).
	 * The widest loads/stores the source alignment allows are used
	 * (ldd/std/ld/st instead of byte or halfword accesses). Casting src16
	 * to wider integer pointers violates C's strict aliasing rules, so the
	 * GCC/Clang may_alias attribute is used to make the casts well-defined.
	 */
	dst = bs->ptr;

#if defined(__GNUC__) || defined(__clang__)
	if (is_aligned(src16, sizeof(uint64_t))) {
		/* 1 ldd + 1 std per 8 output bytes; ~25% faster than 2 aligned path */
		typedef uint64_t __attribute__((may_alias)) alias_u64;
		SPARC_VOLATILE_GCC const alias_u64 *src64 = (const alias_u64 *)(const void *)src16;

		for (i = 0; i < (nb_samples & ~3U); i += 4) {
			uint64_t s0123 = *src64++;

			if (BITSTREAM_IS_CPU_LITTLE_ENDIAN)
				s0123 = (s0123 & 0x000000000000FFFFULL) << 48 |
					(s0123 & 0x00000000FFFF0000ULL) << 16 |
					(s0123 & 0x0000FFFF00000000ULL) >> 16 |
					(s0123 & 0xFFFF000000000000ULL) >> 48;
			put_be64_aligned(dst, s0123);
			dst += sizeof(uint64_t);
		}
	} else if (is_aligned(src16, sizeof(uint32_t))) {
		/* 2 ld + 1 std per 8 output bytes; ~13% faster than 2 aligned path*/
		typedef uint32_t __attribute__((may_alias)) alias_u32;
		const alias_u32 *src32 = (const alias_u32 *)(const void *)src16;

		for (i = 0; i < (nb_samples & ~3U); i += 4) {
			uint32_t s01 = *src32++;
			uint32_t s23 = *src32++;

			if (BITSTREAM_IS_CPU_LITTLE_ENDIAN) {
				s01 = s01 << 16 | s01 >> 16;
				s23 = s23 << 16 | s23 >> 16;
			}
			put_be64_aligned(dst, (uint64_t)s01 << 32 | (uint64_t)s23);
			dst += sizeof(uint64_t);
		}
	} else
#endif /* __GNUC__ || __clang__ */
	{
		/*
		 * src is only 2-byte aligned: Unrolled 8 halfword loads + 4 stores
		 * are the fastest option measured on the GR712RC dev board
		 */
		const uint16_t *src_u16 = (const uint16_t *)src16;
		/* uint32_t const nb_unroll_samples = nb_bulk_samples & ~7U; */

		for (i = 0; i < (nb_samples & ~7U); i += 8) {
			uint32_t s0 = src_u16[i];
			uint32_t s1 = src_u16[i + 1];
			uint32_t s2 = src_u16[i + 2];
			uint32_t s3 = src_u16[i + 3];
			uint32_t s4 = src_u16[i + 4];
			uint32_t s5 = src_u16[i + 5];
			uint32_t s6 = src_u16[i + 6];
			uint32_t s7 = src_u16[i + 7];

			put_be32_aligned(dst, s0 << 16 | s1);
			dst += sizeof(uint32_t);
			put_be32_aligned(dst, s2 << 16 | s3);
			dst += sizeof(uint32_t);
			put_be32_aligned(dst, s4 << 16 | s5);
			dst += sizeof(uint32_t);
			put_be32_aligned(dst, s6 << 16 | s7);
			dst += sizeof(uint32_t);
		}

		/* remaining 4-sample group */
		for (; i < (nb_samples & ~3U); i += 4) {
			uint32_t s0 = src_u16[i];
			uint32_t s1 = src_u16[i + 1];
			uint32_t s2 = src_u16[i + 2];
			uint32_t s3 = src_u16[i + 3];

			put_be32_aligned(dst, s0 << 16 | s1);
			dst += sizeof(uint32_t);
			put_be32_aligned(dst, s2 << 16 | s3);
			dst += sizeof(uint32_t);
		}
	}

	/*
	 * Trailing samples go through the generic bit-by-bit path, so other
	 * bitstream operations work as expected.
	 */
	bs->ptr = dst;
	for (; i < nb_samples; i++)
		bitstream_add_bits32(bs, (uint16_t)src16[i], 16);
}


/**
 * @brief Write an array of 16-bit values (stored in 32-bit containers) as big-endian
 *
 * Extracts the lower 16 bits from each 32-bit value and writes them in big-endian
 * byte order
 *
 * @note This function only works after previous bit writes ended on a 64-bit
 *	 boundary; call bitstream_flush() first if needed
 * @note This function uses sticky error handling. Once an error occurs, subsequent
 *	 calls are ignored. Possible error conditions can be tested with
 *	 bitstream_error() or bitstream_flush().
 *
 * @param bs		pointer to initialised bitstream_writer
 * @param src16_in_32	source buffer of 32-bit values containing 16-bit samples
 *			(native endianness)
 * @param nb_samples	number of samples to write
 */

static __inline void bitstream_add_be16_in_32_array(struct bitstream_writer *bs,
						    const int32_t *src16_in_32, uint32_t nb_samples)
{
	uint32_t i = 0;
	uint8_t *dst;

	if (cmp_is_error_int(bitstream_error(bs)))
		return;

	if (bs->bit_cap != 64) {
		bs->error = CMP_ERROR(INT_BITSTREAM);
		return;
	}

	if (!src16_in_32) {
		bs->error = CMP_ERROR(INT_BITSTREAM);
		return;
	}

	if (!is_aligned(bs->ptr, CMP_DST_ALIGNMENT)) {
		bs->error = CMP_ERROR(INT_BITSTREAM);
		return;
	}

	if (nb_samples > (size_t)(bs->end - bs->ptr) / sizeof(int16_t)) {
		bs->error = CMP_ERROR(DST_TOO_SMALL);
		return;
	}

	/*
	 * This has been tuned for the GR712RC (SPARC V8/LEON3) similarly to
	 * bitstream_add_be16_array().
	 */
	dst = bs->ptr;

#if defined(__GNUC__) || defined(__clang__)
	if (is_aligned(src16_in_32, sizeof(uint64_t))) {
		/* On GR712RC: 15% faster as the 4 byte aligned processing */
		typedef uint64_t __attribute__((may_alias)) alias_u64;
		SPARC_VOLATILE_GCC const alias_u64 *src64 =
			(const alias_u64 *)(const void *)src16_in_32;

		for (; i < (nb_samples & ~3U); i += 4) {
			uint64_t s0, s1, s2, s3;
			uint64_t s01 = *src64++;
			uint64_t s23 = *src64++;

			if (BITSTREAM_IS_CPU_LITTLE_ENDIAN) {
				s01 = s01 >> 32 | s01 << 32;
				s23 = s23 >> 32 | s23 << 32;
			}

			s0 = (s01 >> 32) & 0xFFFFULL;
			s1 = s01 & 0xFFFFULL;
			s2 = (s23 >> 32) & 0xFFFFULL;
			s3 = s23 & 0xFFFFULL;

			put_be64_aligned(dst, s0 << 48 | s1 << 32 | s2 << 16 | s3);
			dst += sizeof(uint64_t);
		}
	}
#endif /* __GNUC__ || __clang__ */

	/*
	 * A (uint16_t *) halfword-load variant is slightly faster on the
	 * GR712RC but non-portable (needs may_alias). As a portable variant is
	 * needed it is not worth maintaining it.
	 */
	for (; i < (nb_samples & ~15U); i += 16) {
		uint32_t const s0 = (uint16_t)src16_in_32[i];
		uint32_t const s1 = (uint16_t)src16_in_32[i + 1];
		uint32_t const s2 = (uint16_t)src16_in_32[i + 2];
		uint32_t const s3 = (uint16_t)src16_in_32[i + 3];
		uint32_t const s4 = (uint16_t)src16_in_32[i + 4];
		uint32_t const s5 = (uint16_t)src16_in_32[i + 5];
		uint32_t const s6 = (uint16_t)src16_in_32[i + 6];
		uint32_t const s7 = (uint16_t)src16_in_32[i + 7];
		uint32_t const s8 = (uint16_t)src16_in_32[i + 8];
		uint32_t const s9 = (uint16_t)src16_in_32[i + 9];
		uint32_t const s10 = (uint16_t)src16_in_32[i + 10];
		uint32_t const s11 = (uint16_t)src16_in_32[i + 11];
		uint32_t const s12 = (uint16_t)src16_in_32[i + 12];
		uint32_t const s13 = (uint16_t)src16_in_32[i + 13];
		uint32_t const s14 = (uint16_t)src16_in_32[i + 14];
		uint32_t const s15 = (uint16_t)src16_in_32[i + 15];

		put_be32_aligned(dst, s0 << 16 | s1);
		dst += sizeof(uint32_t);
		put_be32_aligned(dst, s2 << 16 | s3);
		dst += sizeof(uint32_t);
		put_be32_aligned(dst, s4 << 16 | s5);
		dst += sizeof(uint32_t);
		put_be32_aligned(dst, s6 << 16 | s7);
		dst += sizeof(uint32_t);
		put_be32_aligned(dst, s8 << 16 | s9);
		dst += sizeof(uint32_t);
		put_be32_aligned(dst, s10 << 16 | s11);
		dst += sizeof(uint32_t);
		put_be32_aligned(dst, s12 << 16 | s13);
		dst += sizeof(uint32_t);
		put_be32_aligned(dst, s14 << 16 | s15);
		dst += sizeof(uint32_t);
	}

	/*
	 * Trailing samples go through the generic bit-by-bit path, so other
	 * bitstream operations work as expected.
	 */
	bs->ptr = dst;
	for (; i < nb_samples; i++)
		bitstream_add_bits32(bs, (uint16_t)src16_in_32[i], 16);
}


/**
 * @brief Flushes remaining bits from the internal cache to the buffer
 * Last byte may be padded with zeros
 *
 * @param bs	pointer to an initialised bitstream_writer structure
 *
 * @returns written bytes to bitstream or an error code, which can be checked
 *	using cmp_is_error()
 */

static __inline uint32_t bitstream_flush(struct bitstream_writer *bs)
{
	unsigned int bytes;
	uint8_t *cursor;

	if (cmp_is_error_int(bitstream_error(bs)))
		return bitstream_error(bs);

	cursor = bs->ptr;
	bytes = (64 - bs->bit_cap + 7) / 8;
	if (bytes) {
		uint64_t tmp = bs->cache << bs->bit_cap;

		while (bytes--) {
			if (cursor >= bs->end)
				return bs->error = CMP_ERROR(DST_TOO_SMALL);
			*cursor++ = (uint8_t)(tmp >> (64 - 8));
			tmp <<= 8;
		}
	}

	return (uint32_t)(cursor - bs->start);
}


/**
 * @brief Calculates current total written size in bytes
 *
 * @param bs	pointer to the initialised bitstream_writer structure
 *
 * @returns total bytes effectively written including non-flushed cached bits or
 *	an error code, which can be checked using cmp_is_error()
 */

static __inline uint32_t bitstream_size(const struct bitstream_writer *bs)
{
	if (cmp_is_error_int(bitstream_error(bs)))
		return bitstream_error(bs);

	return (uint32_t)(bs->ptr - bs->start) + ((64 - (uint32_t)bs->bit_cap + 7) / 8);
}


/**
 * @brief Reset the bitstream writer to the beginning of its buffer
 *
 * @param bs	pointer to the initialised bitstream_writer structure
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

static __inline uint32_t bitstream_rewind(struct bitstream_writer *bs)
{
	uint32_t const ret = bitstream_flush(bs);

	if (cmp_is_error_int(ret))
		return ret;

	return bitstream_writer_init(bs, bs->start, (uint32_t)(bs->end - bs->start));
}

#undef SPARC_VOLATILE_GCC
#endif /* CMP_BITSTREAM_WRITER_H */
